/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#include <autoconf.h>

#include <stdlib.h>
#include <string.h>
#include <vka/vka.h>
#include <vka/capops.h>
#include <sel4utils/vspace.h>
#include <sel4utils/vspace_internal.h>

/* For the initial vspace, we must always guarantee we have virtual memory available
 * for each bottom level page table. Future vspaces can then use the initial vspace
 * to allocate bottom level page tables until memory runs out.
 * We have 1 + k + k^2 + ... + k^n number of intermediate paging structures. Where
 * k = VSPACE_LEVEL_SIZE and n = (VSPACE_NUM_LEVELS - 2)
 * We want to calculate this using a geometric sumation. Fortunately we know that
 * VSPACE_LEVEL_SIZE = 2^VSPACE_LEVEL_BITS so when calculating k^n we can simplify to
 * (2^VSPACE_LEVEL_BITS)^n = (2^(VSPACE_LEVEL_BITS * n)) = 1 << (VSPACE_LEVEL_BITS * n) */
#define NUM_MID_LEVEL_STRUCTURES ( (1 - BIT(VSPACE_LEVEL_BITS * (VSPACE_NUM_LEVELS - 1))) / (1 - BIT(VSPACE_LEVEL_BITS)))
/* Number of bottom level structures is just the next term in the previous geometric
 * series, i.e. k^(n + 1) */
#define NUM_BOTTOM_LEVEL_STRUCTURES (BIT(VSPACE_LEVEL_BITS * (VSPACE_NUM_LEVELS - 1)))
/* We need to reserve a range of virtual memory such that we have somewhere to put all of
 * our tables */
#define MID_LEVEL_STRUCTURES_SIZE (NUM_MID_LEVEL_STRUCTURES * sizeof(vspace_mid_level_t))
#define BOTTOM_LEVEL_STRUCTURES_SIZE (NUM_BOTTOM_LEVEL_STRUCTURES * sizeof(vspace_bottom_level_t))
#define VSPACE_RESERVE_SIZE (MID_LEVEL_STRUCTURES_SIZE + BOTTOM_LEVEL_STRUCTURES_SIZE + sizeof(vspace_mid_level_t))
#define VSPACE_RESERVE_START (KERNEL_RESERVED_START - VSPACE_RESERVE_SIZE)

static int
common_init(vspace_t *vspace, vka_t *vka, seL4_CPtr vspace_root,
            vspace_allocated_object_fn allocated_object_fn, void *cookie)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    data->vka = vka;
    data->last_allocated = 0x10000000;
    data->reservation_head = NULL;

    data->vspace_root = vspace_root;
    vspace->allocated_object = allocated_object_fn;
    vspace->allocated_object_cookie = cookie;

    return 0;
}

static void
common_init_post_bootstrap(vspace_t *vspace, sel4utils_map_page_fn map_page)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    /* reserve the kernel region, we do this by marking the
     * top level entry as RESERVED */
    for (int i = TOP_LEVEL_INDEX(KERNEL_RESERVED_START);
             i < VSPACE_LEVEL_SIZE; i++) {
        data->top_level->table[i] = RESERVED;
    }

    data->map_page = map_page;

    /* initialise the rest of the functions now that they are usable */
    vspace->new_pages = sel4utils_new_pages;
    vspace->map_pages = sel4utils_map_pages;
    vspace->new_pages_at_vaddr = sel4utils_new_pages_at_vaddr;
    vspace->map_pages_at_vaddr = sel4utils_map_pages_at_vaddr;
    vspace->unmap_pages = sel4utils_unmap_pages;

    vspace->reserve_range_aligned = sel4utils_reserve_range_aligned;
    vspace->reserve_range_at = sel4utils_reserve_range_at;
    vspace->free_reservation = sel4utils_free_reservation;
    vspace->free_reservation_by_vaddr = sel4utils_free_reservation_by_vaddr;

    vspace->get_cap = sel4utils_get_cap;
    vspace->get_cookie = sel4utils_get_cookie;
    vspace->get_root = sel4utils_get_root;

    vspace->tear_down = sel4utils_tear_down;
    vspace->share_mem_at_vaddr = sel4utils_share_mem_at_vaddr;
}

static void *alloc_and_map(vspace_t *vspace, size_t size) {
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    if ( (size % PAGE_SIZE_4K) != 0) {
        ZF_LOGE("Object must be multiple of base page size");
        return NULL;
    }
    if (data->next_bootstrap_vaddr) {
        void *first_addr = (void*)data->next_bootstrap_vaddr;
        while (size > 0) {
            void *vaddr = (void*)data->next_bootstrap_vaddr;
            vka_object_t frame;
            int error = vka_alloc_frame(data->vka, seL4_PageBits, &frame);
            if (error) {
                LOG_ERROR("Failed to allocate bootstrap frame, error: %d", error);
                return NULL;
            }

            vka_object_t objects[VSPACE_MAP_PAGING_OBJECTS];
            int num = VSPACE_MAP_PAGING_OBJECTS;

            error = sel4utils_map_page(data->vka, data->vspace_root, frame.cptr, vaddr,
                                       seL4_AllRights, 1, objects, &num);

            if (error) {
                vka_free_object(data->vka, &frame);
                LOG_ERROR("Failed to map bootstrap frame at %p, error: %d", vaddr, error);
                return NULL;
            }

            /* Zero the memory */
            memset(vaddr, 0, PAGE_SIZE_4K);

            for (int i = 0; i < num; i++) {
                vspace_maybe_call_allocated_object(vspace, objects[i]);
            }

            data->next_bootstrap_vaddr += PAGE_SIZE_4K;
            size -= PAGE_SIZE_4K;
        }
        return first_addr;
    } else {
        assert(!"not implemented");
    }
    return NULL;
}

static int
reserve_range_bottom(vspace_t *vspace, vspace_bottom_level_t *level, uintptr_t start, uintptr_t end)
{
    while (start < end) {
        int index = INDEX_FOR_LEVEL(start, 0);
        uintptr_t cap = level->cap[index];
        switch (cap) {
        case RESERVED:
            /* nothing to be done */
            break;
        case EMPTY:
            level->cap[index] = RESERVED;
            break;
        default:
            ZF_LOGE("Cannot reserve allocated region");
            return -1;
        }
        start += BYTES_FOR_LEVEL(0);
    }
    return 0;
}

static int
reserve_range_mid(vspace_t *vspace, vspace_mid_level_t *level, int level_num, uintptr_t start, uintptr_t end)
{
    /* walk entries at this level until we complete this range */
    while (start < end) {
        int index = INDEX_FOR_LEVEL(start, level_num);
        /* align the start so we can check for alignment later */
        uintptr_t aligned_start = start & ALIGN_FOR_LEVEL(level_num);
        /* calculate the start of the next index */
        uintptr_t next_start = aligned_start + BYTES_FOR_LEVEL(level_num);
        int must_recurse = 0;
        if (next_start > end) {
            next_start = end;
            must_recurse = 1;
        } else if (start != aligned_start) {
            must_recurse = 1;
        }
        uintptr_t next_table = level->table[index];
        if (next_table == EMPTY) {
            if (must_recurse) {
                /* allocate new level */
                if (level_num == 1) {
                    next_table = (uintptr_t)alloc_and_map(vspace, sizeof(vspace_bottom_level_t));
                } else {
                    next_table = (uintptr_t)alloc_and_map(vspace, sizeof(vspace_mid_level_t));
                }
                if (next_table == EMPTY) {
                    ZF_LOGE("Failed to allocate and map book keeping frames during bootstrapping");
                    return -1;
                }
            } else {
                next_table = RESERVED;
            }
            level->table[index] = next_table;
        }
        /* at this point table is either RESERVED or needs recursion */
        if (next_table != RESERVED) {
            int error;
            if (level_num == 1) {
                error = reserve_range_bottom(vspace, (vspace_bottom_level_t*)next_table, start, next_start);
            } else {
                error = reserve_range_mid(vspace, (vspace_mid_level_t*)next_table, level_num - 1, start, next_start);
            }
            if (error) {
                return error;
            }
        }
        start = next_start;
    }
    return 0;
}

static int
reserve_range(vspace_t *vspace, uintptr_t start, uintptr_t end)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    return reserve_range_mid(vspace, data->top_level, VSPACE_NUM_LEVELS - 1, start, end);
}

/**
 * Symbols in this function need to be provided by your
 * crt0.S or your linker script, such that we can figure out
 * what virtual addresses are taken up by the current task
 */
void
sel4utils_get_image_region(uintptr_t *va_start, uintptr_t *va_end)
{
    extern char __executable_start[];
    extern char _end[];

    *va_start = (uintptr_t) __executable_start;
    *va_end = (uintptr_t) _end;
    *va_end = (uintptr_t) ROUND_UP(*va_end, PAGE_SIZE_4K);
}

static int
reserve_initial_task_regions(vspace_t *vspace, void *existing_frames[])
{

    /* mark the code and data segments as used */
    uintptr_t va_start, va_end;

    sel4utils_get_image_region(&va_start, &va_end);

    /* this is the scope of the virtual memory used by the image, including
     * data, text and stack */
    if (reserve_range(vspace, va_start, va_end)) {
        ZF_LOGE("Error reserving code/data segment");
        return -1;
    }

    /* mark boot info as used */
    if (existing_frames != NULL) {
        for (int i = 0; existing_frames[i] != NULL; i++) {
            if (reserve_range(vspace, (uintptr_t) existing_frames[i], (uintptr_t) existing_frames[i]
                        + PAGE_SIZE_4K)) {
                ZF_LOGE("Error reserving frame at %p", existing_frames[i]);
                return -1;
            }
        }
    }

    return 0;
}

/* What we need to do is bootstrap the book keeping information for our page tables.
 * The whole goal here is that at some point we need to allocate book keeping information
 * and put it somewhere. Putting it somewhere is fine, but we then need to make sure that
 * we track (i.e. mark as used) wherever we ended up putting it. In order to do this we
 * need to allocate memory to create structures to mark etc etc. To prevent this recursive
 * dependency we will mark, right now, as reserved a region large enough such that we could
 * allocate all possible book keeping tables from it */
static int
bootstrap_page_table(vspace_t *vspace)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);

    data->next_bootstrap_vaddr = VSPACE_RESERVE_START;
    /* Allocate top level paging structure */
    data->top_level = alloc_and_map(vspace, sizeof(vspace_mid_level_t));

    /* Try and mark reserved our entire reserve region */
    if (reserve_range(vspace, VSPACE_RESERVE_START, VSPACE_RESERVE_START + VSPACE_RESERVE_SIZE)) {
        return -1;
    }

    return 0;
}

void *
bootstrap_create_level(vspace_t *vspace, size_t size)
{
    return alloc_and_map(vspace, size);
}

/* Interface functions */

int
sel4utils_get_vspace_with_map(vspace_t *loader, vspace_t *new_vspace, sel4utils_alloc_data_t *data,
                              vka_t *vka, seL4_CPtr vspace_root,
                              vspace_allocated_object_fn allocated_object_fn, void *allocated_object_cookie, sel4utils_map_page_fn map_page)
{
    new_vspace->data = (void *) data;

    if (common_init(new_vspace, vka, vspace_root, allocated_object_fn, allocated_object_cookie)) {
        return -1;
    }

    data->bootstrap = loader;

    /* create the top level page table from the loading vspace */
    data->top_level = vspace_new_pages(loader, seL4_AllRights, sizeof(vspace_mid_level_t) / PAGE_SIZE_4K, seL4_PageBits);
    if (data->top_level == NULL) {
        return -1;
    }

    common_init_post_bootstrap(new_vspace, map_page);

    return 0;
}

int
sel4utils_get_vspace(vspace_t *loader, vspace_t *new_vspace, sel4utils_alloc_data_t *data,
                     vka_t *vka, seL4_CPtr vspace_root,
                     vspace_allocated_object_fn allocated_object_fn, void *allocated_object_cookie)
{
    return sel4utils_get_vspace_with_map(loader, new_vspace, data, vka, vspace_root, allocated_object_fn, allocated_object_cookie, sel4utils_map_page_pd);
}

#ifdef CONFIG_VTX
int sel4utils_get_vspace_ept(vspace_t *loader, vspace_t *new_vspace, vka_t *vka,
                             seL4_CPtr ept, vspace_allocated_object_fn allocated_object_fn, void *allocated_object_cookie)
{
    sel4utils_alloc_data_t *data = malloc(sizeof(*data));
    if (!data) {
        return -1;
    }

    return sel4utils_get_vspace_with_map(loader, new_vspace, data, vka, ept, allocated_object_fn, allocated_object_cookie, sel4utils_map_page_ept);
}
#endif /* CONFIG_VTX */

int
sel4utils_bootstrap_vspace(vspace_t *vspace, sel4utils_alloc_data_t *data,
                           seL4_CPtr vspace_root, vka_t *vka,
                           vspace_allocated_object_fn allocated_object_fn, void *cookie, void *existing_frames[])
{
    vspace->data = (void *) data;

    if (common_init(vspace, vka, vspace_root, allocated_object_fn, cookie)) {
        return -1;
    }

    data->bootstrap = NULL;

    if (bootstrap_page_table(vspace)) {
        return -1;
    }

    if (reserve_initial_task_regions(vspace, existing_frames)) {
        return -1;
    }

    common_init_post_bootstrap(vspace, sel4utils_map_page_pd);

    return 0;
}

int
sel4utils_bootstrap_vspace_with_bootinfo(vspace_t *vspace, sel4utils_alloc_data_t *data,
                                         seL4_CPtr vspace_root,
                                         vka_t *vka, seL4_BootInfo *info, vspace_allocated_object_fn allocated_object_fn,
                                         void *allocated_object_cookie)
{
    size_t extra_pages = BYTES_TO_4K_PAGES(info->extraLen);
    uintptr_t extra_base = (uintptr_t)info + PAGE_SIZE_4K;
    void *existing_frames[extra_pages + 3];
    existing_frames[0] = info;
    /* We assume the IPC buffer is less than a page and fits into one page */
    existing_frames[1] = (void *) (seL4_Word)ROUND_DOWN(((seL4_Word)(info->ipcBuffer)), PAGE_SIZE_4K);
    size_t i;
    for (i = 0; i < extra_pages; i++) {
        existing_frames[i + 2] = (void*)(extra_base + i * PAGE_SIZE_4K);
    }
    existing_frames[i + 2] = NULL;

    return sel4utils_bootstrap_vspace(vspace, data, vspace_root, vka, allocated_object_fn,
                                      allocated_object_cookie, existing_frames);
}

int
sel4utils_bootstrap_clone_into_vspace(vspace_t *current, vspace_t *clone, reservation_t image)
{
    sel4utils_res_t *res = reservation_to_res(image);
    seL4_CPtr slot;
    int error = vka_cspace_alloc(get_alloc_data(current)->vka, &slot);

    if (error) {
        return -1;
    }

    cspacepath_t dest;
    vka_cspace_make_path(get_alloc_data(current)->vka, slot, &dest);

    for (uintptr_t page = res->start; page < res->end - 1; page += PAGE_SIZE_4K) {
        /* we don't know if the current vspace has caps to its mappings -
         * it probably doesn't.
         *
         * So we map the page in and copy the data across instead :( */

        /* create the page in the clone vspace */
        error = vspace_new_pages_at_vaddr(clone, (void *) page, 1, seL4_PageBits, image);
        if (error) {
            /* vspace will be left inconsistent */
            ZF_LOGE("Error %d while trying to map page at %"PRIuPTR, error, page);
        }

        seL4_CPtr cap = vspace_get_cap(clone, (void *) page);
        /* copy the cap */
        cspacepath_t src;

        vka_cspace_make_path(get_alloc_data(clone)->vka, cap, &src);
        error = vka_cnode_copy(&dest, &src, seL4_AllRights);
        assert(error == 0);

        /* map a copy of it the current vspace */
        void *dest_addr = vspace_map_pages(current, &dest.capPtr, NULL, seL4_AllRights,
                                           1, seL4_PageBits, 1);
        if (dest_addr == NULL) {
            /* vspace will be left inconsistent */
            ZF_LOGE("Error! Vspace mapping failed, bailing\n");
            return -1;
        }

        /* copy the data */
        memcpy(dest_addr, (void *) page, PAGE_SIZE_4K);

#ifdef CONFIG_ARCH_ARM
        seL4_ARM_Page_Unify_Instruction(dest.capPtr, 0, PAGE_SIZE_4K);
        seL4_ARM_Page_Unify_Instruction(cap, 0, PAGE_SIZE_4K);
#endif /* CONFIG_ARCH_ARM */

        /* unmap our copy */
        vspace_unmap_pages(current, dest_addr, 1, seL4_PageBits, VSPACE_PRESERVE);
        vka_cnode_delete(&dest);
    }

    /* TODO swap out fault handler temporarily to ignore faults here */
    vka_cspace_free(get_alloc_data(current)->vka, slot);
    return 0;
}

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <autoconf.h>

#if defined CONFIG_LIB_SEL4_VSPACE && defined CONFIG_LIB_SEL4_VKA

#include <stdlib.h>
#include <string.h>
#include <vka/vka.h>
#include <vka/capops.h>
#include <sel4utils/vspace.h>
#include <sel4utils/vspace_internal.h>

#ifdef CONFIG_KERNEL_STABLE
#include <sel4/arch/bootinfo.h>
#endif

/* For the initial vspace, we must always guarantee we have virtual memory available
 * for each bottom level page table. Future vspaces can then use the initial vspace
 * to allocate bottom level page tables until memory runs out.
 *
 * The initial vspace then looks like this:
 * 0xffffffff ↑
 *            | kernel code
 * 0xe0000000 ↓
 * 0xdfffffff ↑
 *            | bottom level page tables
 * 0xdf800000 ↓
 * 0xdf7fffff ↑
 *            | top level page table
 * 0xdf7ff000 ↓
 * 0xdf7fefff ↑
 *            | available address space
 * 0x00001000 ↓
 * 0x00000fff ↑
 *            | reserved (null page)
 * 0x00000000 ↓
 *
 * The following constants come from the above layout.
 */

/* reserve enough virtual memory just below the kernel window such that there is enough
 * space to create all possible bottom level structures. This amount of memory is therefore
 * size of second level structure * number of second level structures
 * where number of second level structures is equivalent to number of bits in the top level translation */
#define FIRST_BOTTOM_LEVEL_PAGE_TABLE_VADDR (KERNEL_RESERVED_START - sizeof(bottom_level_t) * VSPACE_LEVEL_SIZE)
/* we also need some memory for top level structure. place that just below all our bottom
   level page tables. no good way of specifying the size, will assume one page */
#define TOP_LEVEL_PAGE_TABLE_VADDR (FIRST_BOTTOM_LEVEL_PAGE_TABLE_VADDR - PAGE_SIZE_4K)
/* when bootstrapping we need to allocate 1 bottom level page table to store the top level page table
 * index in, and enough bottom levels to store indexes of all possible bottom levels */
#define INITIAL_BOTTOM_LEVEL_PAGES 6
#define INITIAL_BOTTOM_LEVEL_TABLES (INITIAL_BOTTOM_LEVEL_PAGES / PAGES_FOR_BOTTOM_LEVEL)

/* sanity checks - check our constants above are what we say they are */
#if !defined(CONFIG_X86_64) && !defined(CONFIG_PAE_PAGING)
compile_time_assert(sel4utils_vspace_1, TOP_LEVEL_INDEX(TOP_LEVEL_PAGE_TABLE_VADDR) == 893);
compile_time_assert(sel4utils_vspace_2, BOTTOM_LEVEL_INDEX(TOP_LEVEL_PAGE_TABLE_VADDR) == 1023);
compile_time_assert(sel4utils_vspace_3, TOP_LEVEL_INDEX(FIRST_BOTTOM_LEVEL_PAGE_TABLE_VADDR) == 894);
compile_time_assert(sel4utils_vspace_4, BOTTOM_LEVEL_INDEX(FIRST_BOTTOM_LEVEL_PAGE_TABLE_VADDR) == 0);
#endif

/* check our data structures are 4K*2 page sized like the rest of the code assumes they are */
compile_time_assert(bottom_level_8k, sizeof(bottom_level_t) == (PAGE_SIZE_4K * 2));
compile_time_assert(top_level_4k, sizeof(bottom_level_t *) * VSPACE_LEVEL_SIZE == PAGE_SIZE_4K);

static int
common_init(vspace_t *vspace, vka_t *vka, seL4_CPtr page_directory,
            vspace_allocated_object_fn allocated_object_fn, void *cookie)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    data->vka = vka;
    data->last_allocated = (void *) 0x10000000;
    data->reservation_head = NULL;

    data->page_directory = page_directory;
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
            i <= TOP_LEVEL_INDEX(KERNEL_RESERVED_END); i++) {
        data->top_level[i] = (bottom_level_t *) RESERVED;
    }

    data->map_page = map_page;

    /* initialise the rest of the functions now that they are useable */
    vspace->new_pages = sel4utils_new_pages;
    vspace->new_pages_at_vaddr = sel4utils_new_pages_at_vaddr;

    vspace->map_pages = sel4utils_map_pages;
    vspace->map_pages_at_vaddr = sel4utils_map_pages_at_vaddr;
    vspace->unmap_pages = sel4utils_unmap_pages;

    vspace->reserve_range = sel4utils_reserve_range;
    vspace->reserve_range_at = sel4utils_reserve_range_at;
    vspace->free_reservation = sel4utils_free_reservation;
    vspace->free_reservation_by_vaddr = sel4utils_free_reservation_by_vaddr;

    vspace->get_cap = sel4utils_get_cap;
    vspace->get_cookie = sel4utils_get_cookie;
    vspace->get_root = sel4utils_get_root;

    vspace->tear_down = sel4utils_tear_down;
}

static int
reserve_range(vspace_t *vspace, void *start, void *end)
{
    int pages = BYTES_TO_4K_PAGES(ROUND_UP((uint32_t) ((seL4_Word)end), PAGE_SIZE_4K) -
                                  ROUND_DOWN((uint32_t) ((seL4_Word)start), PAGE_SIZE_4K));

    int error = 0;
    for (int i = 0; i < pages && error == 0; i++) {
        error = reserve_entries(vspace, start + (i * PAGE_SIZE_4K) , seL4_PageBits);
    }

    if (error) {
        LOG_ERROR("Error reserving range between %p and %p", start, end);
    }

    return error;
}

/**
 * Symbols in this function need to be provided by your
 * crt0.S or your linker script, such that we can figure out
 * what virtual addresses are taken up by the current task
 */
void
sel4utils_get_image_region(seL4_Word *va_start, seL4_Word *va_end)
{
    extern char __executable_start[];
    extern char _end[];

    *va_start = (seL4_Word) __executable_start;
    *va_end = (seL4_Word) _end;
    *va_end = (seL4_Word) (seL4_Word)ROUND_UP((uint32_t) ((seL4_Word) * va_end), (uint32_t) PAGE_SIZE_4K);
}


static int
reserve_initial_task_regions(vspace_t *vspace, void *existing_frames[])
{

    /* mark the code and data segments as used */
    seL4_Word va_start, va_end;

    sel4utils_get_image_region(&va_start, &va_end);

    /* this is the scope of the virtual memory used by the image, including
     * data, text and stack */
    if (reserve_range(vspace, (void *) va_start, (void *) va_end)) {
        LOG_ERROR("Error reserving code/data segment");
        return -1;
    }

    /* mark boot info as used */
    if (existing_frames != NULL) {
        for (int i = 0; existing_frames[i] != NULL; i++) {
            if (reserve_range(vspace, existing_frames[i], existing_frames[i] + PAGE_SIZE_4K)) {
                LOG_ERROR("Error reserving frame at %p", existing_frames[i]);
                return -1;
            }
        }
    }

    return 0;
}

static int
alloc_and_map_bootstrap_frame(vspace_t *vspace, vka_object_t *frame, void *vaddr)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);

    int error = vka_alloc_frame(data->vka, seL4_PageBits, frame);
    if (error) {
        LOG_ERROR("Failed to allocate bootstrap frame, error: %d", error);
        return error;
    }

    vka_object_t objects[3];
    int num = 3;

    error = sel4utils_map_page(data->vka, data->page_directory, frame->cptr, vaddr,
                               seL4_AllRights, 1, objects, &num);

    if (error) {
        vka_free_object(data->vka, frame);
        LOG_ERROR("Failed to map bootstrap frame at %p, error: %d", vaddr, error);
        return error;
    }

    /* Zero the memory */
    memset(vaddr, 0, PAGE_SIZE_4K);

    for (int i = 0; i < num; i++) {
        vspace_maybe_call_allocated_object(vspace, objects[i]);
    }

    return seL4_NoError;
}

int
bootstrap_create_level(vspace_t *vspace, void *vaddr)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    UNUSED int error;

    void *pt_vaddr = data->next_bottom_level_vaddr;
    for (int i = 0; i < PAGES_FOR_BOTTOM_LEVEL; i++) {
        vka_object_t bottom_level = {0};

        if (alloc_and_map_bootstrap_frame(vspace, &bottom_level, pt_vaddr) != seL4_NoError) {
            LOG_ERROR("Failed to bootstrap a level, everything is broken.");
            /* leak memory, can't really recover */
            return -1;
        }

        error = update(vspace, pt_vaddr, bottom_level.cptr, bottom_level.ut);
        /* this cannot fail */
        assert(error == seL4_NoError);
        pt_vaddr += PAGE_SIZE_4K;
    }

    data->top_level[TOP_LEVEL_INDEX(vaddr)] = (bottom_level_t *) data->next_bottom_level_vaddr;
    data->next_bottom_level_vaddr = pt_vaddr;

    return 0;
}

static int
bootstrap_page_table(vspace_t *vspace)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);

    /* First we allocate a page for the top level page table to live in */
    vka_object_t top_level = {0};
    if (alloc_and_map_bootstrap_frame(vspace, &top_level, (void *) TOP_LEVEL_PAGE_TABLE_VADDR)) {
        return -1;
    }

    /* set up the pointer to top level page table */
    data->top_level = (bottom_level_t **) TOP_LEVEL_PAGE_TABLE_VADDR;

    vka_object_t bottom_levels[INITIAL_BOTTOM_LEVEL_PAGES];
    void *vaddr = (void *) FIRST_BOTTOM_LEVEL_PAGE_TABLE_VADDR;
    for (int i = 0; i < INITIAL_BOTTOM_LEVEL_PAGES; i++) {
        if (alloc_and_map_bootstrap_frame(vspace, &bottom_levels[i], vaddr)) {
            return -1;
        }
        vaddr += PAGE_SIZE_4K;
    }

    /* set up pointers to bottom level page tables */
    vaddr = (void *) FIRST_BOTTOM_LEVEL_PAGE_TABLE_VADDR;
    for (int i = 0; i < INITIAL_BOTTOM_LEVEL_TABLES; i++) {
        data->top_level[TOP_LEVEL_INDEX(TOP_LEVEL_PAGE_TABLE_VADDR) + i] = vaddr;
        vaddr += (PAGES_FOR_BOTTOM_LEVEL * PAGE_SIZE_4K);
    }

    /* now update those entries in our new page tables */
    /* these cannot fail - failure is only caused by lack of a bottom level page table,
     * and we just allocated them. */
    UNUSED int error = update(vspace, (void *) TOP_LEVEL_PAGE_TABLE_VADDR, top_level.cptr, top_level.ut);
    assert(error == seL4_NoError);

    vaddr = (void *) FIRST_BOTTOM_LEVEL_PAGE_TABLE_VADDR;
    for (int i = 0; i < INITIAL_BOTTOM_LEVEL_PAGES; i++) {
        error = update(vspace, vaddr, bottom_levels[i].cptr, bottom_levels[i].ut);
        assert(error == seL4_NoError);
        vaddr += PAGE_SIZE_4K;
    }

    vaddr = (void *) FIRST_BOTTOM_LEVEL_PAGE_TABLE_VADDR + (INITIAL_BOTTOM_LEVEL_PAGES * PAGE_SIZE_4K);
    data->next_bottom_level_vaddr = vaddr;

    /* reserve the rest of them */
    for (int i = INITIAL_BOTTOM_LEVEL_PAGES; i < VSPACE_LEVEL_SIZE * PAGES_FOR_BOTTOM_LEVEL; i++) {
        assert(data->top_level[TOP_LEVEL_INDEX(vaddr)] != NULL);
        error = reserve(vspace, vaddr);
        assert(error == seL4_NoError);
        vaddr += PAGE_SIZE_4K;
    }

    return 0;
}

/* Interface functions */

int
sel4utils_get_vspace_with_map(vspace_t *loader, vspace_t *new_vspace, sel4utils_alloc_data_t *data,
                              vka_t *vka, seL4_CPtr page_directory,
                              vspace_allocated_object_fn allocated_object_fn, void *allocated_object_cookie, sel4utils_map_page_fn map_page)
{
    new_vspace->data = (void *) data;

    if (common_init(new_vspace, vka, page_directory, allocated_object_fn, allocated_object_cookie)) {
        return -1;
    }

    data->bootstrap = loader;

    /* create the top level page table from the loading vspace */
    data->top_level = (bottom_level_t **) vspace_new_pages(loader, seL4_AllRights, 1, seL4_PageBits);
    if (data->top_level == NULL) {
        return -1;
    }

    common_init_post_bootstrap(new_vspace, map_page);

    return 0;
}

int
sel4utils_get_vspace(vspace_t *loader, vspace_t *new_vspace, sel4utils_alloc_data_t *data,
                     vka_t *vka, seL4_CPtr page_directory,
                     vspace_allocated_object_fn allocated_object_fn, void *allocated_object_cookie)
{
    return sel4utils_get_vspace_with_map(loader, new_vspace, data, vka, page_directory, allocated_object_fn, allocated_object_cookie, sel4utils_map_page_pd);
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
                           seL4_CPtr page_directory, vka_t *vka,
                           vspace_allocated_object_fn allocated_object_fn, void *cookie, void *existing_frames[])
{
    vspace->data = (void *) data;

    if (common_init(vspace, vka, page_directory, allocated_object_fn, cookie)) {
        return -1;
    }

    data->bootstrap = NULL;

    if (bootstrap_page_table(vspace)) {
        return -1;
    }

    reserve_initial_task_regions(vspace, existing_frames);

    common_init_post_bootstrap(vspace, sel4utils_map_page_pd);

    return 0;
}


int
sel4utils_bootstrap_vspace_with_bootinfo(vspace_t *vspace, sel4utils_alloc_data_t *data,
                                         seL4_CPtr page_directory,
                                         vka_t *vka, seL4_BootInfo *info, vspace_allocated_object_fn allocated_object_fn,
                                         void *allocated_object_cookie)
{
    void *existing_frames[] = {
        (void *) info,
#if defined(CONFIG_ARCH_IA32) && defined(CONFIG_KERNEL_STABLE)
        (void *) seL4_IA32_GetBootInfo(),
#endif
        /* We assume the IPC buffer is less than a page and fits into one page */
        (void *) (seL4_Word)ROUND_DOWN((uint32_t) ((seL4_Word)(info->ipcBuffer)), PAGE_SIZE_4K),
        NULL
    };

    return sel4utils_bootstrap_vspace(vspace, data, page_directory, vka, allocated_object_fn,
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

    for (void *page = res->start; page < res->end - 1; page += PAGE_SIZE_4K) {
        /* we don't know if the current vspace has caps to its mappings -
         * it probably doesn't.
         *
         * So we map the page in and copy the data across instead :( */

        /* create the page in the clone vspace */
        int error = vspace_new_pages_at_vaddr(clone, page, 1, seL4_PageBits, image);
        if (error) {
            /* vspace will be left inconsistent */
            LOG_ERROR("Error %d while trying to map page at %p\n", error, page);
        }

        seL4_CPtr cap = vspace_get_cap(clone, page);
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
            LOG_ERROR("Error! Vspace mapping failed, bailing\n");
            return -1;
        }

        /* copy the data */
        memcpy(dest_addr, page, PAGE_SIZE_4K);

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


#endif /* CONFIG_LIB_SEL4_VSPACE && CONFIG_LIB_SEL4_VKA */

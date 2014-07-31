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
 * 0xdfc00000 ↓
 * 0xdfbfffff ↑
 *            | top level page table
 * 0xdfbff000 ↓
 * 0xdfbfefff ↑
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
/* reserve the first page to catch null pointer dereferences */
#define FIRST_VADDR (0 + PAGE_SIZE_4K)

/* sanity checks - check our constants above are what we say they are */
#ifndef CONFIG_X86_64
compile_time_assert(sel4utils_vspace_1, TOP_LEVEL_INDEX(TOP_LEVEL_PAGE_TABLE_VADDR) == 894);
compile_time_assert(sel4utils_vspace_2, BOTTOM_LEVEL_INDEX(TOP_LEVEL_PAGE_TABLE_VADDR) == 1023);
compile_time_assert(sel4utils_vspace_3, TOP_LEVEL_INDEX(FIRST_BOTTOM_LEVEL_PAGE_TABLE_VADDR) == 895);
compile_time_assert(sel4utils_vspace_4, BOTTOM_LEVEL_INDEX(FIRST_BOTTOM_LEVEL_PAGE_TABLE_VADDR) == 0);
#endif

/* check our data structures are 4K page sized like the rest of the code assumes they are */
compile_time_assert(bottom_level_4k, sizeof(bottom_level_t) == PAGE_SIZE_4K);
compile_time_assert(top_level_4k, sizeof(bottom_level_t *) * VSPACE_LEVEL_SIZE == PAGE_SIZE_4K);

static int
common_init(vspace_t *vspace, vka_t *vka, seL4_CPtr page_directory,
            vspace_allocated_object_fn allocated_object_fn, void *cookie)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    data->vka = vka;
    data->last_allocated = (void *) 0x10000000;

    vspace->page_directory = page_directory;
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
    vspace->new_stack = sel4utils_new_stack;
    vspace->free_stack = sel4utils_free_stack;

    vspace->new_ipc_buffer = sel4utils_new_ipc_buffer;
    vspace->free_ipc_buffer = sel4utils_free_ipc_buffer;

    vspace->new_pages = sel4utils_new_pages;
    vspace->new_pages_at_vaddr = sel4utils_new_pages_at_vaddr;
    vspace->free_pages = sel4utils_free_pages;

    vspace->map_pages = sel4utils_map_pages;
    vspace->map_pages_at_vaddr = sel4utils_map_pages_at_vaddr;
    vspace->unmap_pages = sel4utils_unmap_pages;
    vspace->unmap_reserved_pages = sel4utils_unmap_reserved_pages;

    vspace->reserve_range = sel4utils_reserve_range;
    vspace->reserve_range_at = sel4utils_reserve_range_at;
    vspace->free_reservation = sel4utils_free_reservation;

    vspace->get_cap = sel4utils_get_cap;
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
sel4utils_get_image_region(seL4_Word *va_start, seL4_Word *va_end) {
    extern char __executable_start[];
    extern char _end[];

    *va_start = (seL4_Word) __executable_start;
    *va_end = (seL4_Word) _end;
    *va_end = (seL4_Word) (seL4_Word)ROUND_UP((uint32_t) ((seL4_Word)*va_end), (uint32_t) PAGE_SIZE_4K);
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

    vka_object_t pagetable = {0};

    error = sel4utils_map_page(data->vka, vspace->page_directory, frame->cptr, vaddr,
            seL4_AllRights, 1, &pagetable);

    if (error) {
        LOG_ERROR("Failed to map bootstrap frame at %p, error: %d", vaddr, error);
        return error;
    }

    /* Zero the memory */
    memset(vaddr, 0, PAGE_SIZE_4K);

    if (pagetable.cptr != 0) {
        vspace_maybe_call_allocated_object(vspace, pagetable);
    }

    return seL4_NoError;
}

int
bootstrap_create_level(vspace_t *vspace, void *vaddr)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);

    vka_object_t bottom_level = {0};
    if (alloc_and_map_bootstrap_frame(vspace, &bottom_level,
            data->next_bottom_level_vaddr) != seL4_NoError) {
        return -1;
    }

    int error __attribute__((unused)) = update(vspace, data->next_bottom_level_vaddr, bottom_level.cptr);
    /* this cannot fail */
    assert(error == seL4_NoError);

    data->top_level[TOP_LEVEL_INDEX(vaddr)] = (bottom_level_t *) data->next_bottom_level_vaddr;

    data->next_bottom_level_vaddr += PAGE_SIZE_4K;

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

    /* The top level page table vaddr is the last entry in the second last bottom level
     * page table - create that level and map it in*/
    vka_object_t first_bottom_level = {0};
    if (alloc_and_map_bootstrap_frame(vspace, &first_bottom_level,
            (void *) FIRST_BOTTOM_LEVEL_PAGE_TABLE_VADDR)) {
        return -1;
    }

    /* The level we just created is the first entry in the last bottom level page table -
     * create that level and map it in */
    vka_object_t second_bottom_level = {0};
    if (alloc_and_map_bootstrap_frame(vspace, &second_bottom_level,
            (void *) FIRST_BOTTOM_LEVEL_PAGE_TABLE_VADDR + PAGE_SIZE_4K)) {
        return -1;
    }

    /* set up the pointers to our new page tables */
    data->top_level = (bottom_level_t **) TOP_LEVEL_PAGE_TABLE_VADDR;
    data->top_level[TOP_LEVEL_INDEX(TOP_LEVEL_PAGE_TABLE_VADDR)] =
        (bottom_level_t *) FIRST_BOTTOM_LEVEL_PAGE_TABLE_VADDR;
    data->top_level[TOP_LEVEL_INDEX(FIRST_BOTTOM_LEVEL_PAGE_TABLE_VADDR)] =
        (bottom_level_t *) (FIRST_BOTTOM_LEVEL_PAGE_TABLE_VADDR + PAGE_SIZE_4K);

    /* now update those entries in our new page tables */
    /* these cannot fail - failure is only caused by lack of a bottom level page table,
     * and we just allocated them. */
    int error = update(vspace, (void *) TOP_LEVEL_PAGE_TABLE_VADDR, top_level.cptr);
    (void)error;
    assert(error == seL4_NoError);
    error = update(vspace, (void *) FIRST_BOTTOM_LEVEL_PAGE_TABLE_VADDR, first_bottom_level.cptr);
    assert(error == seL4_NoError);
    error = update(vspace, (void *) FIRST_BOTTOM_LEVEL_PAGE_TABLE_VADDR + PAGE_SIZE_4K,
                   second_bottom_level.cptr);
    assert(error == seL4_NoError);

    /* Finally reserve the rest of the entries for the rest of the bottom level page tables */
    void *vaddr = (void *) FIRST_BOTTOM_LEVEL_PAGE_TABLE_VADDR;
    data->next_bottom_level_vaddr = vaddr + (2 * PAGE_SIZE_4K);
    for (int i = 2; i < VSPACE_LEVEL_SIZE; i++) {
        error = reserve(vspace, vaddr + (i * PAGE_SIZE_4K));
        assert(error == seL4_NoError);
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
sel4utils_bootstrap_clone_into_vspace(vspace_t *current, vspace_t *clone, reservation_t *image)
{  
    seL4_CPtr slot;
    int error = vka_cspace_alloc(get_alloc_data(current)->vka, &slot);

    if (error) {
        return -1;
    }

    cspacepath_t dest;
    vka_cspace_make_path(get_alloc_data(current)->vka, slot, &dest);
   
    for (void *page = image->start; page < image->end - 1; page += PAGE_SIZE_4K) {
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
        void *dest_addr = vspace_map_pages(current, &dest.capPtr, seL4_AllRights, 1, seL4_PageBits, 1);
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
        vspace_unmap_pages(current, dest_addr, 1, seL4_PageBits);
        vka_cnode_delete(&dest);
    }

    /* TODO swap out fault handler temporarily to ignore faults here */
    vka_cspace_free(get_alloc_data(current)->vka, slot);
    return 0;
}


#endif /* CONFIG_LIB_SEL4_VSPACE && CONFIG_LIB_SEL4_VKA */

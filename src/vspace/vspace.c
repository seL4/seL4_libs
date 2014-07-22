/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/* see sel4utils/vspace.h for details */
#include <autoconf.h>

#if defined CONFIG_LIB_SEL4_VKA && defined CONFIG_LIB_SEL4_VSPACE

#include <stdlib.h>
#include <string.h>

#include <sel4utils/vspace.h>

#include <sel4utils/vspace_internal.h>

#include <utils/util.h>

/* see comment in vspace_internal for why these checks exist */
compile_time_assert(pages_are_4k1, BOTTOM_LEVEL_BITS_OFFSET == seL4_PageBits);
compile_time_assert(pages_are_4k2, BIT(BOTTOM_LEVEL_BITS_OFFSET) == PAGE_SIZE_4K);

static int
create_level(vspace_t *vspace, void* vaddr)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);

    /* We need a level in the bootstrapper vspace */
    if (data->bootstrap == NULL) {
        return bootstrap_create_level(vspace, vaddr);
    }

    /* Otherwise we allocate our level out of the bootstrapper vspace -
     * which is where bookkeeping is mapped */
    bottom_level_t *bottom_level = vspace_new_pages(data->bootstrap, seL4_AllRights, 1, seL4_PageBits);
    if (bottom_level == NULL) {
        return -1;
    }
    memset(bottom_level, 0, BIT(seL4_PageBits));

    data->top_level[TOP_LEVEL_INDEX(vaddr)] = bottom_level;

    return 0;
}

int
assure_level(vspace_t *vspace, void *vaddr)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);

    assert(data->top_level != NULL);
    assert(data->top_level != (void *) RESERVED);

    if (data->top_level[TOP_LEVEL_INDEX(vaddr)] == NULL) {
        int error = create_level(vspace, vaddr);
        if (error) {
            return -1;
        }
    }

    return 0;
}

static int
check_empty_range(bottom_level_t *top_level[], void *vaddr, size_t num_pages, size_t size_bits)
{
    int result = 1;
    int num_4k_pages = BYTES_TO_4K_PAGES(num_pages * (1 << size_bits));
    for (int i = 0; i < num_4k_pages && result; i++) {
        /* a range is empty if it does not have a mapped cap and it isn't reserved */
        result = is_available(top_level, vaddr + (i * PAGE_SIZE_4K))
                 && !is_reserved(top_level, vaddr + (i * PAGE_SIZE_4K));
    }

    return result;
}

/* check a range of pages is all reserved */
static int
check_reserved_range(bottom_level_t *top_level[], void *vaddr, size_t num_pages, size_t size_bits)
{
    int result = 1;
    int num_4k_pages = BYTES_TO_4K_PAGES(num_pages * (1 << size_bits));
    for (int i = 0; i < num_4k_pages && result; i++) {
        /* a range is empty if it does not have a mapped cap and it isn't reserved */
        result = is_reserved(top_level, vaddr + (i * PAGE_SIZE_4K));
       
    }

    return result;
}

/* check that vaddr is actually in the reservation */
static int
check_reservation_bounds(reservation_t *reservation, void *vaddr, size_t num_pages, size_t size_bits)
{
    return (vaddr >= reservation->start) &&
           (vaddr + (num_pages * (1 << size_bits))) <= reservation->end;
}

static int
check_reservation(bottom_level_t *top_level[], reservation_t *reservation, void *vaddr,
                  size_t num_pages, size_t size_bits)
{
    return check_reservation_bounds(reservation, vaddr, num_pages, size_bits) &
           check_reserved_range(top_level, vaddr, num_pages, size_bits);
}

static void
perform_reservation(vspace_t *vspace, reservation_t *reservation, void *vaddr, size_t bytes,
                    seL4_CapRights rights, int cacheable)
{
    reservation->start = (void *) (seL4_Word)ROUND_DOWN((uint32_t) ((seL4_Word)vaddr), PAGE_SIZE_4K);
    reservation->end = (void *) (seL4_Word)ROUND_UP((uint32_t) ((seL4_Word)vaddr) + bytes, PAGE_SIZE_4K);

    reservation->rights = rights;
    reservation->cacheable = cacheable;

    int error = seL4_NoError;
    void *v = reservation->start;

    for (v = reservation->start; v < reservation->end &&
            error == seL4_NoError; v += PAGE_SIZE_4K) {
        error = reserve(vspace, v);
    }

    /* return the amount we sucessfully reserved */
    reservation->end = v;
}

int
sel4utils_map_page_pd(vspace_t *vspace, seL4_CPtr cap, void *vaddr, seL4_CapRights rights,
         int cacheable, size_t size_bits)
{
    vka_object_t pagetable = {0};
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);

    int error = sel4utils_map_page(data->vka, vspace->page_directory, cap, vaddr,
                rights, cacheable, &pagetable);
    if (error) {
        /* everything has gone to hell. Do no clean up. */
        LOG_ERROR("Error mapping pages, bailing: %d", error);
        return -1;
    }

    if (pagetable.cptr != 0) {
        vspace_maybe_call_allocated_object(vspace, pagetable);
    }
    pagetable.cptr = 0;

    return seL4_NoError;
}

#ifdef CONFIG_VTX
int
sel4utils_map_page_ept(vspace_t *vspace, seL4_CPtr cap, void *vaddr, seL4_CapRights rights,
        int cacheable, size_t size_bits)
{
    struct sel4utils_alloc_data *data = get_alloc_data(vspace);
    vka_object_t pagetable = {0};
    vka_object_t pagedir = {0};

    int error = sel4utils_map_ept_page(data->vka, vspace->page_directory, cap,
                (seL4_Word) vaddr, rights, cacheable, size_bits, &pagetable, &pagedir);
    if (error) {
        LOG_ERROR("Error mapping pages, bailing\n");
        return -1;
    }

    if (pagetable.cptr != 0) {
        vspace_maybe_call_allocated_object(vspace, pagetable);
        pagetable.cptr = 0;
    }

    if (pagedir.cptr != 0) {
        vspace_maybe_call_allocated_object(vspace, pagedir);
        pagedir.cptr = 0;
    }

    return seL4_NoError;
}
#endif /* CONFIG_VTX */

static int
map_page(vspace_t *vspace, seL4_CPtr cap, void *vaddr, seL4_CapRights rights,
        int cacheable, size_t size_bits)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    return data->map_page(vspace, cap, vaddr, rights, cacheable, size_bits);
}

static void *
find_range(sel4utils_alloc_data_t *data, size_t num_pages, size_t size_bits)
{
    /* look for a contiguous range that is free.
     * We use first-fit with the optimisation that we store
     * a pointer to the last thing we freed/allocated */
    int num_4k_pages = BYTES_TO_4K_PAGES(num_pages * (1 << size_bits));
    void *current = data->last_allocated;
    size_t contiguous = 0;
    void *start = data->last_allocated;
    while (contiguous < num_4k_pages) {

        if (is_available(data->top_level, current) && IS_ALIGNED((uint32_t)((seL4_Word)start), size_bits)) {
            contiguous++;
        } else {
            start = current + PAGE_SIZE_4K;
            contiguous = 0;
        }

        current += PAGE_SIZE_4K;

        if (current >= (void *) KERNEL_RESERVED_START) {
            LOG_ERROR("Out of virtual memory");
            return NULL;
        }

    }

    data->last_allocated = current;

    return start;
}

static int
alloc_pages(vspace_t *vspace, seL4_CPtr pages[], size_t num_pages, size_t size_bits)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    for (int i = 0; i < num_pages; i++) {
        vka_object_t object;
        if (vka_alloc_frame(data->vka, size_bits, &object) != 0) {
            /* abort! */
            LOG_ERROR("Failed to allocate page");
            return -1;
        }

        vspace_maybe_call_allocated_object(vspace, object);
        pages[i] = object.cptr;
    }

    return 0;
}

static int
map_pages_at_vaddr(vspace_t *vspace, seL4_CPtr caps[], void *vaddr, size_t num_pages,
                   size_t size_bits, seL4_CapRights rights, int cacheable)
{
    int error = seL4_NoError;
    for (int i = 0; i < num_pages && error == seL4_NoError; i++) {
        assert(caps[i] != 0);
        error = map_page(vspace, caps[i], vaddr, rights, cacheable, size_bits);

        if (error == seL4_NoError) {
            error = update_entries(vspace, vaddr, caps[i], size_bits);
            vaddr += (1 << size_bits);
        }
    }
    return error;
}

static int
new_pages_at_vaddr(vspace_t *vspace, void *vaddr, size_t num_pages, size_t size_bits,
                   seL4_CapRights rights, int cacheable)
{
    seL4_CPtr pages[num_pages];

    int error = alloc_pages(vspace, pages, num_pages, size_bits);
    if (error == 0) {
        error = map_pages_at_vaddr(vspace, pages, vaddr, num_pages, size_bits, rights, cacheable);
    }

    return error;
}

/* VSPACE INTERFACE FUNCTIONS */

int
sel4utils_map_pages_at_vaddr(vspace_t *vspace, seL4_CPtr caps[], void *vaddr,
        size_t num_pages, size_t size_bits, reservation_t *reservation)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);

    if (!check_reservation(data->top_level, reservation, vaddr, num_pages, size_bits)) {
        return -1;
    }

    return map_pages_at_vaddr(vspace, caps, vaddr, num_pages, size_bits,
            reservation->rights, reservation->cacheable);
}

seL4_CPtr
sel4utils_get_cap(vspace_t *vspace, void *vaddr)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    seL4_CPtr entry = get_entry(data->top_level, vaddr);

    if (entry == RESERVED) {
        entry = 0;
    }
    return entry;
}

void *
sel4utils_map_pages(vspace_t *vspace, seL4_CPtr caps[], seL4_CapRights rights,
                    size_t num_pages, size_t size_bits, int cacheable)
{
    struct sel4utils_alloc_data *data = get_alloc_data(vspace);
    void *vaddr = find_range(data, num_pages, size_bits);

    if (vaddr == NULL) {
        return NULL;
    }

    int error = map_pages_at_vaddr(vspace, caps, vaddr, num_pages, size_bits, rights,
                cacheable);
    if (error == seL4_NoError) {
        return vaddr;
    } else {
        return NULL;
    }
}


int sel4utils_unmap_reserved_pages(vspace_t *vspace, void *vaddr, size_t num_pages, size_t size_bits, reservation_t *reservation) {
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);

    if (!check_reservation_bounds(reservation, vaddr, num_pages, size_bits)) {
        printf("check reservation failed vaddr %p\n", vaddr);
        return -1;
    }
    for (int i = 0; i < num_pages; i++) {
        seL4_CPtr cap = sel4utils_get_cap(vspace, vaddr);

        if (cap != 0) {
            int error = seL4_ARCH_Page_Unmap(get_entry(data->top_level, vaddr));
            if (error != seL4_NoError) {
                LOG_ERROR("Failed to unmap page at vaddr %p", vaddr);
            }

            /*update entires*/
            update_entries(vspace, vaddr, RESERVED, size_bits);
        }

        vaddr += (1 << size_bits);
    }

    return 0;
}

void
sel4utils_unmap_pages(vspace_t *vspace, void *vaddr, size_t num_pages, size_t size_bits)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    for (int i = 0; i < num_pages; i++) {
        seL4_CPtr cap = sel4utils_get_cap(vspace, vaddr);

        if (cap != 0) {
            int error = seL4_ARCH_Page_Unmap(get_entry(data->top_level, vaddr));
            if (error != seL4_NoError) {
                LOG_ERROR("Failed to unmap page at vaddr %p", vaddr);
            }
        }

        vaddr += (1 << size_bits);
    }
}

int
sel4utils_new_pages_at_vaddr(vspace_t *vspace, void *vaddr, size_t num_pages,
        size_t size_bits, reservation_t *reservation)
{
    struct sel4utils_alloc_data *data = get_alloc_data(vspace);

    if (!check_reservation(data->top_level, reservation, vaddr, num_pages, size_bits)) {
        LOG_ERROR("Range for vaddr %p with "DFMT" 4k pages not reserved!", vaddr, num_pages);
        return -1;
    }

    return new_pages_at_vaddr(vspace, vaddr, num_pages, size_bits,
            reservation->rights, reservation->cacheable);
}

void *
sel4utils_new_pages(vspace_t *vspace, seL4_CapRights rights, size_t num_pages,
                    size_t size_bits)
{
    struct sel4utils_alloc_data *data = get_alloc_data(vspace);

    void *vaddr = find_range(data, num_pages, size_bits);
    if (vaddr == NULL) {
        return NULL;
    }

    int error = new_pages_at_vaddr(vspace, vaddr, num_pages, size_bits, rights, 1);
    if (error == seL4_NoError) {
        return vaddr;
    } else {
        return NULL;
    }
}

void
sel4utils_free_pages(vspace_t *vspace, void *vaddr, size_t num_pages,
        size_t size_bits)
{
    /* first unmap the pages */
    sel4utils_unmap_pages(vspace, vaddr, num_pages, size_bits);

    /* now mark them all as free */
    for (int i = 0; i < num_pages; i++) {
        clear_entries(vspace, vaddr, size_bits);
        vaddr += PAGE_SIZE_4K;
    }
}

void *
sel4utils_new_stack(vspace_t *vspace)
{
    /* this implementation allocates stacks with small pages. */
    int num_pages = BYTES_TO_4K_PAGES(CONFIG_SEL4UTILS_STACK_SIZE);
    struct sel4utils_alloc_data *data = get_alloc_data(vspace);

    void *vaddr = find_range(data, num_pages + 1, seL4_PageBits);

    /* reserve the first page as the guard */
    int error = reserve(vspace, vaddr);
    if (error) {
        return NULL;
    }

    void *stack_bottom = vaddr + PAGE_SIZE_4K;
    error = new_pages_at_vaddr(vspace, stack_bottom, num_pages, seL4_PageBits,
            seL4_AllRights, 1);

    /* abort */
    if (error != seL4_NoError) {
        clear(vspace, vaddr);
        return NULL;
    }

    /* return a pointer to the TOP of the stack */
    return stack_bottom + CONFIG_SEL4UTILS_STACK_SIZE;
}

void
sel4utils_free_stack(vspace_t *vspace, void *stack_top)
{
    int num_pages = BYTES_TO_4K_PAGES(CONFIG_SEL4UTILS_STACK_SIZE);

    sel4utils_free_pages(vspace, stack_top - CONFIG_SEL4UTILS_STACK_SIZE,
            num_pages, seL4_PageBits);

    /* unreserve the guard page */
    clear(vspace, stack_top - CONFIG_SEL4UTILS_STACK_SIZE - PAGE_SIZE_4K);
}

void *
sel4utils_new_ipc_buffer(vspace_t *vspace, seL4_CPtr *page)
{
    void *vaddr = sel4utils_new_pages(vspace, seL4_AllRights, 1, seL4_PageBits);
    if (vaddr == NULL) {
        LOG_ERROR("Failed to create ipc buffer");
        return NULL;
    }

    *page = sel4utils_get_cap(vspace, vaddr);

    return vaddr;
}

void
sel4utils_free_ipc_buffer(vspace_t *vspace, void *vaddr)
{
    sel4utils_free_pages(vspace, vaddr, 1, seL4_PageBits);
}


int sel4utils_reserve_range_no_alloc(vspace_t *vspace, reservation_t *reservation, size_t size,
        seL4_CapRights rights, int cacheable, void **result)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    void *vaddr = find_range(data, BYTES_TO_4K_PAGES(size), seL4_PageBits);

    if (vaddr == NULL) {
        return -1;
    }

    *result = vaddr;
    perform_reservation(vspace, reservation, vaddr, size, rights, cacheable);
    return 0;
}

reservation_t *
sel4utils_reserve_range(vspace_t *vspace, size_t size, seL4_CapRights rights,
        int cacheable, void **result)
{
    reservation_t *reservation = malloc(sizeof(struct reservation));
    if (reservation == NULL) {
        LOG_ERROR("Malloc failed");
        return NULL;
    }

    int error = sel4utils_reserve_range_no_alloc(vspace, reservation, size, rights, cacheable, result);
    if (error) {
        free(reservation);
        return NULL;
    }

    return reservation;
}

int sel4utils_reserve_range_at_no_alloc(vspace_t *vspace, reservation_t *reservation, void *vaddr,
        size_t size, seL4_CapRights rights, int cacheable)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    if (!check_empty_range(data->top_level, vaddr, BYTES_TO_4K_PAGES(size),
            seL4_PageBits)) {
        LOG_ERROR("Range not available at %p, size 0x"XFMT"", vaddr, size);
        return -1;
    }
    perform_reservation(vspace, reservation, vaddr, size, rights, cacheable);
    return 0;
}

reservation_t *
sel4utils_reserve_range_at(vspace_t *vspace, void *vaddr, size_t size, seL4_CapRights
        rights, int cacheable)
{
    reservation_t *reservation = malloc(sizeof(struct reservation));
    if (reservation == NULL) {
        LOG_ERROR("Malloc failed");
        return NULL;
    }

    int error = sel4utils_reserve_range_at_no_alloc(vspace, reservation, vaddr, size, rights, cacheable);
    if (error) {
        free(reservation);
        return NULL;
    }
    return reservation;
}

void sel4utils_free_reservation_no_alloc(vspace_t *vspace, reservation_t *reservation)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    for (void *current = reservation->start; current < reservation->end; current += PAGE_SIZE_4K) {
        if (get_entry(data->top_level, current) == RESERVED) {
            clear(vspace, current);
        }
    }
}

void
sel4utils_free_reservation(vspace_t *vspace, reservation_t *reservation)
{
    sel4utils_free_reservation_no_alloc(vspace, reservation);
    free(reservation);
}

#endif /* CONFIG_LIB_SEL4_VKA && CONFIG_LIB_SEL4_VSPACE */

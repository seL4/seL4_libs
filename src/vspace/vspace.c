/*
 * Copyright 014, NICTA
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

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <sel4utils/vspace.h>

#include <sel4utils/vspace_internal.h>
#include <vka/capops.h>

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
    bottom_level_t *bottom_level = vspace_new_pages(data->bootstrap, seL4_AllRights,
                                                    PAGES_FOR_BOTTOM_LEVEL, seL4_PageBits);
    if (bottom_level == NULL) {
        return -1;
    }
    memset(bottom_level, 0, sizeof(bottom_level_t));

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
check_reservation_bounds(sel4utils_res_t *reservation, void *vaddr, size_t num_pages, size_t size_bits)
{
    return (vaddr >= reservation->start) &&
           (vaddr + (num_pages * (1 << size_bits))) <= reservation->end;
}

static int
check_reservation(bottom_level_t *top_level[], sel4utils_res_t *reservation, void *vaddr,
                  size_t num_pages, size_t size_bits)
{
    return check_reservation_bounds(reservation, vaddr, num_pages, size_bits) &
           check_reserved_range(top_level, vaddr, num_pages, size_bits);
}

static void
insert_reservation(sel4utils_alloc_data_t *data, sel4utils_res_t *reservation)
{

    assert(data != NULL);
    assert(reservation != NULL);

    reservation->next = NULL;

    /* insert at head */
    if (data->reservation_head == NULL || reservation->start > data->reservation_head->start) {
        reservation->next = data->reservation_head;
        data->reservation_head = reservation;
        return;
    }

    /* insert elsewhere */
    sel4utils_res_t *prev = data->reservation_head;
    sel4utils_res_t *current = prev->next;

    while (current != NULL) {
        /* insert in the middle */
        if (reservation->start > current->start) {
            reservation->next = current;
            prev->next = reservation;
            return;
        }
        prev = current;
        current = current->next;
    }

    /* insert at the end */
    prev->next = reservation;
}

static void
remove_reservation(sel4utils_alloc_data_t *data, sel4utils_res_t *reservation)
{
    /* remove head */
    if (reservation == data->reservation_head) {
        data->reservation_head = data->reservation_head->next;
        reservation->next = NULL;
        return;
    }

    sel4utils_res_t *prev = data->reservation_head;
    sel4utils_res_t *current = prev->next;

    while (current != NULL) {
        /* remove middle */
        if (current == reservation) {
            prev->next = reservation->next;
            reservation->next = NULL;
            return;
        }
        prev = current;
        current = current->next;
    }

    /* remove tail */
    prev->next = NULL;
    reservation->next = NULL;
}

static void
perform_reservation(vspace_t *vspace, sel4utils_res_t *reservation, void *vaddr, size_t bytes,
                    seL4_CapRights rights, int cacheable)
{
    assert(reservation != NULL);

    reservation->start = (void *) (seL4_Word) ROUND_DOWN((uint32_t) ((seL4_Word)vaddr),
                                                         PAGE_SIZE_4K);
    reservation->end = (void *) (seL4_Word) ROUND_UP((uint32_t) ((seL4_Word)vaddr) + bytes,
                                                     PAGE_SIZE_4K);

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

    /* insert the reservation ordered */
    insert_reservation(get_alloc_data(vspace), reservation);
}

int
sel4utils_map_page_pd(vspace_t *vspace, seL4_CPtr cap, void *vaddr, seL4_CapRights rights,
                      int cacheable, size_t size_bits)
{
    vka_object_t objects[3];
    int num = 3;
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);

    int error = sel4utils_map_page(data->vka, data->page_directory, cap, vaddr,
                                   rights, cacheable, objects, &num);
    if (error) {
        /* everything has gone to hell. Do no clean up. */
        LOG_ERROR("Error mapping pages, bailing: %d", error);
        return -1;
    }

    for (int i = 0; i < num; i++) {
        vspace_maybe_call_allocated_object(vspace, objects[i]);
    }

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

    int error = sel4utils_map_ept_page(data->vka, data->page_directory, cap,
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

#ifdef CONFIG_IOMMU
int
sel4utils_map_page_iommu(vspace_t *vspace, seL4_CPtr cap, void *vaddr, seL4_CapRights rights,
                         int cacheable, size_t size_bits)
{
    struct sel4utils_alloc_data *data = get_alloc_data(vspace);
    int num_pts = 0;
    /* The maximum number of page table levels current intel hardware implements is 6 */
    vka_object_t pts[7];

    int error = sel4utils_map_iospace_page(data->vka, data->page_directory, cap,
                                           (seL4_Word) vaddr, rights, cacheable, size_bits, pts, &num_pts);
    if (error) {
        LOG_ERROR("Error mapping pages, bailing");
        return -1;
    }

    for (int i = 0; i < num_pts; i++) {
        vspace_maybe_call_allocated_object(vspace, pts[i]);
    }

    return seL4_NoError;
}
#endif /* CONFIG_IOMMU */

static int
map_page(vspace_t *vspace, seL4_CPtr cap, void *vaddr, seL4_CapRights rights,
         int cacheable, size_t size_bits)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    return data->map_page(vspace, cap, vaddr, rights, cacheable, size_bits);
}

static sel4utils_res_t *
find_reserve(sel4utils_alloc_data_t *data, void *vaddr)
{

    sel4utils_res_t *current = data->reservation_head;

    while (current != NULL) {
        if (vaddr >= current->start && vaddr < current->end) {
            return current;
        }

        current = current->next;
    }

    return NULL;
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
map_pages_at_vaddr(vspace_t *vspace, seL4_CPtr caps[], uint32_t cookies[],
                   void *vaddr, size_t num_pages,
                   size_t size_bits, seL4_CapRights rights, int cacheable)
{
    int error = seL4_NoError;
    for (int i = 0; i < num_pages && error == seL4_NoError; i++) {
        assert(caps[i] != 0);
        error = map_page(vspace, caps[i], vaddr, rights, cacheable, size_bits);

        if (error == seL4_NoError) {
            uint32_t cookie = cookies == NULL ? 0 : cookies[i];
            error = update_entries(vspace, vaddr, caps[i], size_bits, cookie);
            vaddr += (1 << size_bits);
        }
    }

    return error;
}

static int
new_pages_at_vaddr(vspace_t *vspace, void *vaddr, size_t num_pages, size_t size_bits,
                   seL4_CapRights rights, int cacheable)
{

    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    int i;
    int error = seL4_NoError;
    void *start_vaddr = vaddr;

    for (i = 0; i < num_pages; i++) {
        vka_object_t object;
        if (vka_alloc_frame(data->vka, size_bits, &object) != 0) {
            /* abort! */
            LOG_ERROR("Failed to allocate page");
            break;
        }

        error = map_page(vspace, object.cptr, vaddr, rights, cacheable, size_bits);

        if (error == seL4_NoError) {
            error = update_entries(vspace, vaddr, object.cptr, size_bits, object.ut);
            vaddr += (1 << size_bits);
        } else {
            vka_free_object(data->vka, &object);
            break;
        }

    }

    if (i < num_pages) {
        /* we failed, clean up successfully allocated pages */
        sel4utils_unmap_pages(vspace, start_vaddr, i, size_bits, data->vka);
    }

    return error;
}

/* VSPACE INTERFACE FUNCTIONS */

int
sel4utils_map_pages_at_vaddr(vspace_t *vspace, seL4_CPtr caps[], uint32_t cookies[], void *vaddr,
                             size_t num_pages, size_t size_bits, reservation_t reservation)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    sel4utils_res_t *res = reservation_to_res(reservation);

    if (!check_reservation(data->top_level, res, vaddr, num_pages, size_bits)) {
        return -1;
    }

    return map_pages_at_vaddr(vspace, caps, cookies, vaddr, num_pages, size_bits,
                              res->rights, res->cacheable);
}

seL4_CPtr
sel4utils_get_cap(vspace_t *vspace, void *vaddr)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    seL4_CPtr cap = get_cap(data->top_level, vaddr);

    if (cap == RESERVED) {
        cap = 0;
    }
    return cap;
}

seL4_CPtr
sel4utils_get_cookie(vspace_t *vspace, void *vaddr)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    return get_cookie(data->top_level, vaddr);
}

void *
sel4utils_map_pages(vspace_t *vspace, seL4_CPtr caps[], uint32_t cookies[], seL4_CapRights rights,
                    size_t num_pages, size_t size_bits, int cacheable)
{
    struct sel4utils_alloc_data *data = get_alloc_data(vspace);
    void *vaddr = find_range(data, num_pages, size_bits);

    if (vaddr == NULL) {
        return NULL;
    }

    int error = map_pages_at_vaddr(vspace, caps, cookies, vaddr, num_pages, size_bits, rights,
                                   cacheable);
    if (error == seL4_NoError) {
        return vaddr;
    } else {
        return NULL;
    }
}

void
sel4utils_unmap_pages(vspace_t *vspace, void *vaddr, size_t num_pages, size_t size_bits, vka_t *vka)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    sel4utils_res_t *reserve = find_reserve(data, vaddr);

    if (vka == VSPACE_FREE) {
        vka = data->vka;
    }

    for (int i = 0; i < num_pages; i++) {
        seL4_CPtr cap = sel4utils_get_cap(vspace, vaddr);

        /* unmap */
        if (cap != 0) {
            int error = seL4_ARCH_Page_Unmap(get_cap(data->top_level, vaddr));
            if (error != seL4_NoError) {
                LOG_ERROR("Failed to unmap page at vaddr %p", vaddr);
            }
        }

        if (vka) {
            cspacepath_t path;
            vka_cspace_make_path(vka, cap, &path);
            vka_cnode_delete(&path);
            vka_cspace_free(vka, cap);
            vka_utspace_free(vka, kobject_get_type(KOBJECT_FRAME, size_bits),
                             size_bits, sel4utils_get_cookie(vspace, vaddr));
        }

        if (reserve == NULL) {
            clear_entries(vspace, vaddr, size_bits);
        } else {
            reserve_entries(vspace, vaddr, size_bits);
        }

        vaddr += (1 << size_bits);
    }
}

int
sel4utils_new_pages_at_vaddr(vspace_t *vspace, void *vaddr, size_t num_pages,
                             size_t size_bits, reservation_t reservation)
{
    struct sel4utils_alloc_data *data = get_alloc_data(vspace);
    sel4utils_res_t *res = reservation_to_res(reservation);

    if (!check_reservation(data->top_level, res, vaddr, num_pages, size_bits)) {
        LOG_ERROR("Range for vaddr %p with %"PRIuPTR" 4k pages not reserved!", vaddr, num_pages);
        return -1;
    }

    return new_pages_at_vaddr(vspace, vaddr, num_pages, size_bits, res->rights, res->cacheable);
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


int sel4utils_reserve_range_no_alloc(vspace_t *vspace, sel4utils_res_t *reservation, size_t size,
                                     seL4_CapRights rights, int cacheable, void **result)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    void *vaddr = find_range(data, BYTES_TO_4K_PAGES(size), seL4_PageBits);

    if (vaddr == NULL) {
        return -1;
    }

    *result = vaddr;
    reservation->malloced = 0;
    perform_reservation(vspace, reservation, vaddr, size, rights, cacheable);
    return 0;
}

reservation_t
sel4utils_reserve_range(vspace_t *vspace, size_t size, seL4_CapRights rights,
                        int cacheable, void **result)
{
    reservation_t reservation;
    sel4utils_res_t *res = (sel4utils_res_t *) malloc(sizeof(sel4utils_res_t));

    if (res == NULL) {
        LOG_ERROR("Malloc failed");
        reservation.res = NULL;
        return reservation;
    }

    reservation.res = res;
    res->malloced = 1;

    int error = sel4utils_reserve_range_no_alloc(vspace, res, size, rights, cacheable, result);
    if (error) {
        free(reservation.res);
        reservation.res = NULL;
    }

    return reservation;
}

int sel4utils_reserve_range_at_no_alloc(vspace_t *vspace, sel4utils_res_t *reservation, void *vaddr,
                                        size_t size, seL4_CapRights rights, int cacheable)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    if (!check_empty_range(data->top_level, vaddr, BYTES_TO_4K_PAGES(size),
                           seL4_PageBits)) {
        LOG_ERROR("Range not available at %p, size %p", vaddr, (void*)size);
        return -1;
    }
    perform_reservation(vspace, reservation, vaddr, size, rights, cacheable);
    return 0;
}

reservation_t
sel4utils_reserve_range_at(vspace_t *vspace, void *vaddr, size_t size, seL4_CapRights
                           rights, int cacheable)
{
    reservation_t reservation;
    reservation.res = malloc(sizeof(sel4utils_res_t));

    if (reservation.res == NULL) {
        LOG_ERROR("Malloc failed");
        reservation.res = NULL;
        return reservation;
    }

    int error = sel4utils_reserve_range_at_no_alloc(vspace, reservation.res, vaddr, size, rights, cacheable);
    if (error) {
        free(reservation.res);
        reservation.res = NULL;
    }
    return reservation;
}

void sel4utils_free_reservation_no_alloc(vspace_t *vspace, sel4utils_res_t *res)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);

    for (void *current = res->start; current < res->end; current += PAGE_SIZE_4K) {
        if (get_cap(data->top_level, current) == RESERVED) {
            clear(vspace, current);
        }
    }
    remove_reservation(data, res);
}

void
sel4utils_free_reservation(vspace_t *vspace, reservation_t reservation)
{
    sel4utils_free_reservation_no_alloc(vspace, reservation.res);
    free(reservation.res);
}

void
sel4utils_free_reservation_by_vaddr(vspace_t *vspace, void *vaddr)
{

    reservation_t reservation;
    reservation.res = (void *) find_reserve(get_alloc_data(vspace), vaddr);
    sel4utils_free_reservation(vspace, reservation);
}

int
sel4utils_move_resize_reservation(vspace_t *vspace, reservation_t reservation, void *vaddr,
                                  size_t bytes)
{
    assert(reservation.res != NULL);
    sel4utils_res_t *res = reservation.res;
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);

    void *new_start = (void *) (seL4_Word) ROUND_DOWN((uint32_t) ((seL4_Word)vaddr), PAGE_SIZE_4K);
    void *new_end = (void *) (seL4_Word) ROUND_UP((uint32_t) ((seL4_Word)vaddr) + bytes,
                                                  PAGE_SIZE_4K);
    void *v = NULL;

    /* Sanity checks that newly asked reservation space is available. */
    if (new_start < res->start) {
        size_t prepending_region_size = (char*) res->start - (char*) new_start;
        if (!check_empty_range(data->top_level, new_start,
                               BYTES_TO_4K_PAGES(prepending_region_size), seL4_PageBits)) {
            return -1;
        }
    }
    if (new_end > res->end) {
        size_t appending_region_size = (char*) new_end - (char*) res->end;
        if (!check_empty_range(data->top_level, res->end,
                               BYTES_TO_4K_PAGES(appending_region_size), seL4_PageBits)) {
            return -2;
        }
    }

    for (v = new_start; v < new_end; v += PAGE_SIZE_4K) {
        if (v < res->start || v >= res->end) {
            /* Any outside the reservation must be unreserved. */
            int error UNUSED = reserve(vspace, v);
            /* Should not cause any errors as we have just checked the regions are free. */
            assert(!error);
        } else {
            v = res->end - PAGE_SIZE_4K;
        }
    }

    for (v = res->start; v < res->end; v += PAGE_SIZE_4K) {
        if (v < new_start || v >= new_end) {
            /* Clear any regions that aren't reserved by the new region any more. */
            if (get_cap(data->top_level, v) == RESERVED) {
                clear(vspace, v);
            }
        } else {
            v = new_end - PAGE_SIZE_4K;
        }
    }

    char need_reinsert = 0;
    if (res->start != new_start) {
        need_reinsert = 1;
    }

    res->start = new_start;
    res->end = new_end;

    /* We may need to re-insert the reservation into the list to keep it sorted by start address. */
    if (need_reinsert) {
        remove_reservation(data, res);
        insert_reservation(data, res);
    }

    return 0;
}

seL4_CPtr
sel4utils_get_root(vspace_t *vspace)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    return data->page_directory;
}


void
sel4utils_tear_down(vspace_t *vspace, vka_t *vka)
{

    sel4utils_alloc_data_t *data = get_alloc_data(vspace);

    if (data->bootstrap == NULL) {
        LOG_ERROR("Not implemented: sel4utils cannot currently tear down a self-bootstrapped vspace\n");
        return;
    }

    if (vka == VSPACE_FREE) {
        vka = data->vka;
    }

    /* free all the reservations */
    while (data->reservation_head != NULL) {
        sel4utils_res_t *res = data->reservation_head;
        sel4utils_free_reservation_no_alloc(vspace, res);
        if (res->malloced) {
            free(res);
        }
    }

    /* now clear all the pages in the vspace (not the page tables) */
    uint32_t start = TOP_LEVEL_INDEX(FIRST_VADDR);
    uint32_t end = TOP_LEVEL_INDEX(KERNEL_RESERVED_START) + 1;
    uint32_t idx;

    for (idx = start; idx < end; idx++) {
        bottom_level_t *bottom_level = data->top_level[idx];

        if (bottom_level != NULL && (uint32_t) bottom_level != RESERVED) {
            /* free all of the pages in the vspace */
            for (uint32_t i = 0; i < VSPACE_LEVEL_SIZE; i++) {
                uint32_t page4k = 1;
                uint32_t cookie = bottom_level->cookies[i];
                uintptr_t vaddr = (idx << TOP_LEVEL_BITS_OFFSET) | (i << BOTTOM_LEVEL_BITS_OFFSET);
                /* if the cookie isn't 0 we free the object/frame */
                if (cookie != 0) {
                    /* we might be unmapping a large page, figure out how big it
                     * is by looking for consecutive, identical entries */
                    for (uint32_t j = i + 1; j < VSPACE_LEVEL_SIZE && bottom_level->cookies[j] == cookie; j++, page4k++);
                    sel4utils_unmap_pages(vspace, (void*)vaddr, 1, PAGE_BITS_4K * page4k, vka);
                }
            }
            /* now free the level we were using */
            vspace_unmap_pages(data->bootstrap, bottom_level, PAGES_FOR_BOTTOM_LEVEL, PAGE_BITS_4K,
                               VSPACE_FREE);
        }
    }

    /* now free the top level */
    vspace_unmap_pages(data->bootstrap, data->top_level, 1, PAGE_BITS_4K, VSPACE_FREE);

}



#endif /* CONFIG_LIB_SEL4_VKA && CONFIG_LIB_SEL4_VSPACE */

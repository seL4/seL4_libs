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

/* see sel4utils/vspace.h for details */
#include <autoconf.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <sel4utils/vspace.h>
#include <sel4utils/page.h>

#include <sel4utils/vspace_internal.h>
#include <vka/capops.h>

#include <utils/util.h>

void *
create_level(vspace_t *vspace, size_t size)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);

    /* We need a level in the bootstrapper vspace */
    if (data->bootstrap == NULL) {
        return bootstrap_create_level(vspace, size);
    }

    /* Otherwise we allocate our level out of the bootstrapper vspace -
     * which is where bookkeeping is mapped */
    void *level = vspace_new_pages(data->bootstrap, seL4_AllRights,
                                   size / PAGE_SIZE_4K, seL4_PageBits);
    if (level == NULL) {
        return NULL;
    }
    memset(level, 0, size);

    return level;
}

/* check that vaddr is actually in the reservation */
static int
check_reservation_bounds(sel4utils_res_t *reservation, uintptr_t start, uintptr_t end)
{
    return start >= reservation->start &&
           end <= reservation->end;
}

static int
check_reservation(vspace_mid_level_t *top_level, sel4utils_res_t *reservation, uintptr_t start, uintptr_t end)
{
    return check_reservation_bounds(reservation, start,end) &&
           is_reserved_range(top_level, start,end);
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
perform_reservation(vspace_t *vspace, sel4utils_res_t *reservation, uintptr_t vaddr, size_t bytes,
                    seL4_CapRights_t rights, int cacheable)
{
    assert(reservation != NULL);

    UNUSED int error;
    reservation->start = ROUND_DOWN(vaddr, PAGE_SIZE_4K);
    reservation->end = ROUND_UP(vaddr + bytes, PAGE_SIZE_4K);

    reservation->rights = rights;
    reservation->cacheable = cacheable;

    error = reserve_entries_range(vspace, reservation->start, reservation->end, true);

    /* only support to reserve things that we've checked that we can */
    assert(error == seL4_NoError);

    /* insert the reservation ordered */
    insert_reservation(get_alloc_data(vspace), reservation);
}

int
sel4utils_map_page_pd(vspace_t *vspace, seL4_CPtr cap, void *vaddr, seL4_CapRights_t rights,
                      int cacheable, size_t size_bits)
{
    vka_object_t objects[VSPACE_MAP_PAGING_OBJECTS];
    int num = VSPACE_MAP_PAGING_OBJECTS;
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);

    int error = sel4utils_map_page(data->vka, data->vspace_root, cap, vaddr,
                                   rights, cacheable, objects, &num);
    if (error) {
        /* everything has gone to hell. Do no clean up. */
        ZF_LOGE("Error mapping pages, bailing: %d", error);
        return -1;
    }

    for (int i = 0; i < num; i++) {
        vspace_maybe_call_allocated_object(vspace, objects[i]);
    }

    return seL4_NoError;
}

#ifdef CONFIG_VTX
int
sel4utils_map_page_ept(vspace_t *vspace, seL4_CPtr cap, void *vaddr, seL4_CapRights_t rights,
                       int cacheable, size_t size_bits)
{
    struct sel4utils_alloc_data *data = get_alloc_data(vspace);
    vka_object_t pagetable = {0};
    vka_object_t pagedir = {0};
    vka_object_t pdpt = {0};

    int error = sel4utils_map_ept_page(data->vka, data->vspace_root, cap,
                                       (seL4_Word) vaddr, rights, cacheable, size_bits, &pagetable, &pagedir, &pdpt);
    if (error) {
        ZF_LOGE("Error mapping pages, bailing\n");
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

    if (pdpt.cptr != 0) {
        vspace_maybe_call_allocated_object(vspace, pdpt);
        pdpt.cptr = 0;
    }

    return seL4_NoError;
}
#endif /* CONFIG_VTX */

#ifdef CONFIG_IOMMU
int
sel4utils_map_page_iommu(vspace_t *vspace, seL4_CPtr cap, void *vaddr, seL4_CapRights_t rights,
                         int cacheable, size_t size_bits)
{
    struct sel4utils_alloc_data *data = get_alloc_data(vspace);
    int num_pts = 0;
    /* The maximum number of page table levels current Intel hardware implements is 6 */
    vka_object_t pts[7];

    int error = sel4utils_map_iospace_page(data->vka, data->vspace_root, cap,
                                           (seL4_Word) vaddr, rights, cacheable, size_bits, pts, &num_pts);
    if (error) {
        ZF_LOGE("Error mapping pages, bailing");
        return -1;
    }

    for (int i = 0; i < num_pts; i++) {
        vspace_maybe_call_allocated_object(vspace, pts[i]);
    }

    return seL4_NoError;
}
#endif /* CONFIG_IOMMU */

static int
map_page(vspace_t *vspace, seL4_CPtr cap, void *vaddr, seL4_CapRights_t rights,
         int cacheable, size_t size_bits)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    return data->map_page(vspace, cap, vaddr, rights, cacheable, size_bits);
}

static sel4utils_res_t *
find_reserve(sel4utils_alloc_data_t *data, uintptr_t vaddr)
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
    size_t contiguous = 0;
    uintptr_t start = ALIGN_UP(data->last_allocated, SIZE_BITS_TO_BYTES(size_bits));
    uintptr_t current = start;

    assert(IS_ALIGNED(start, size_bits));
    while (contiguous < num_pages) {

        bool available = is_available(data->top_level, current, size_bits);
        current += SIZE_BITS_TO_BYTES(size_bits);

        if (available) {
            /* keep going! */
            contiguous++;
        } else {
            /* reset start and try again */
            start = current;
            contiguous = 0;
        }

        if (current >= KERNEL_RESERVED_START) {
            ZF_LOGE("Out of virtual memory");
            return NULL;
        }

    }

    data->last_allocated = current;

    return (void *) start;
}

static int
map_pages_at_vaddr(vspace_t *vspace, seL4_CPtr caps[], uintptr_t cookies[],
                   void *vaddr, size_t num_pages,
                   size_t size_bits, seL4_CapRights_t rights, int cacheable)
{
    int error = seL4_NoError;

    for (int i = 0; i < num_pages && error == seL4_NoError; i++) {
        error = map_page(vspace, caps[i], vaddr, rights, cacheable, size_bits);

        if (error == seL4_NoError) {
            uintptr_t cookie = cookies == NULL ? 0 : cookies[i];
            error = update_entries(vspace, (uintptr_t) vaddr, caps[i], size_bits, cookie);
            vaddr = (void *) ((uintptr_t) vaddr + (BIT(size_bits)));
        }
    }
    return error;
}

static int
new_pages_at_vaddr(vspace_t *vspace, void *vaddr, size_t num_pages, size_t size_bits,
                   seL4_CapRights_t rights, int cacheable, bool can_use_dev)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    int i;
    int error = seL4_NoError;
    void *start_vaddr = vaddr;

    for (i = 0; i < num_pages; i++) {
        vka_object_t object;
        if (vka_alloc_frame_maybe_device(data->vka, size_bits, can_use_dev, &object) != 0) {
            /* abort! */
            ZF_LOGE("Failed to allocate page number: %d out of %zu", i, num_pages);
            error = seL4_NotEnoughMemory;
            break;
        }

        error = map_page(vspace, object.cptr, vaddr, rights, cacheable, size_bits);

        if (error == seL4_NoError) {
            error = update_entries(vspace, (uintptr_t) vaddr, object.cptr, size_bits, object.ut);
            vaddr = (void *) ((uintptr_t) vaddr + (BIT(size_bits)));
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
sel4utils_map_pages_at_vaddr(vspace_t *vspace, seL4_CPtr caps[], uintptr_t cookies[], void *vaddr,
                             size_t num_pages, size_t size_bits, reservation_t reservation)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    sel4utils_res_t *res = reservation_to_res(reservation);

    if (!sel4_valid_size_bits(size_bits)) {
        ZF_LOGE("Invalid size_bits %zu", size_bits);
        return -1;
    }

    if (!check_reservation(data->top_level, res, (uintptr_t) vaddr, (uintptr_t)vaddr + num_pages * BIT(size_bits))) {
        ZF_LOGE("Invalid reservation");
        return -1;
    }

    return map_pages_at_vaddr(vspace, caps, cookies, vaddr, num_pages, size_bits,
                              res->rights, res->cacheable);
}

void *
sel4utils_map_pages(vspace_t *vspace, seL4_CPtr caps[], uintptr_t cookies[],
                    seL4_CapRights_t rights, size_t num_pages, size_t size_bits,
                    int cacheable)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    int error;
    void *ret_vaddr;

    assert(num_pages > 0);

    ret_vaddr = find_range(data, num_pages, size_bits);
    if (ret_vaddr == NULL) {
        return NULL;
    }

    error = map_pages_at_vaddr(vspace, caps, cookies,
                               ret_vaddr, num_pages, size_bits,
                               rights, cacheable);
    if (error != 0) {
        if (clear_entries(vspace, (uintptr_t)ret_vaddr, size_bits) != 0) {
            ZF_LOGE("FATAL: Failed to clear VMM metadata for vmem @0x%p, %lu pages.",
                    ret_vaddr, BIT(size_bits));
            /* This is probably cause for a panic, but continue anyway. */
        }
        return NULL;
    }
    return ret_vaddr;
}

seL4_CPtr
sel4utils_get_cap(vspace_t *vspace, void *vaddr)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    seL4_CPtr cap = get_cap(data->top_level, (uintptr_t) vaddr);

    if (cap == RESERVED) {
        cap = 0;
    }
    return cap;
}

uintptr_t
sel4utils_get_cookie(vspace_t *vspace, void *vaddr)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    return get_cookie(data->top_level, (uintptr_t) vaddr);
}

void
sel4utils_unmap_pages(vspace_t *vspace, void *vaddr, size_t num_pages, size_t size_bits, vka_t *vka)
{
    uintptr_t v = (uintptr_t) vaddr;
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    sel4utils_res_t *reserve = find_reserve(data, v);

    if (!sel4_valid_size_bits(size_bits)) {
        ZF_LOGE("Invalid size_bits %zu", size_bits);
        return;
    }

    if (vka == VSPACE_FREE) {
        vka = data->vka;
    }

    for (int i = 0; i < num_pages; i++) {
        seL4_CPtr cap = get_cap(data->top_level, v);

        /* unmap */
        if (cap != 0) {
            int error = seL4_ARCH_Page_Unmap(cap);
            if (error != seL4_NoError) {
                ZF_LOGE("Failed to unmap page at vaddr %p", vaddr);
            }
        }

        if (vka) {
            cspacepath_t path;
            vka_cspace_make_path(vka, cap, &path);
            vka_cnode_delete(&path);
            vka_cspace_free(vka, cap);
            if (sel4utils_get_cookie(vspace, vaddr)) {
                vka_utspace_free(vka, kobject_get_type(KOBJECT_FRAME, size_bits),
                                     size_bits, sel4utils_get_cookie(vspace, vaddr));
            }
        }

        if (reserve == NULL) {
            clear_entries(vspace, v, size_bits);
        } else {
            reserve_entries(vspace, v, size_bits);
        }
        assert(get_cap(data->top_level, v) != cap);
        assert(get_cookie(data->top_level, v) == 0);

        v += (BIT(size_bits));
        vaddr = (void *) v;
    }
}

int
sel4utils_new_pages_at_vaddr(vspace_t *vspace, void *vaddr, size_t num_pages,
                             size_t size_bits, reservation_t reservation, bool can_use_dev)
{
    struct sel4utils_alloc_data *data = get_alloc_data(vspace);
    sel4utils_res_t *res = reservation_to_res(reservation);

    if (!check_reservation(data->top_level, res, (uintptr_t) vaddr, (uintptr_t)vaddr + num_pages * BIT(size_bits))) {
        ZF_LOGE("Range for vaddr %p with %"PRIuPTR" 4k pages not reserved!", vaddr, num_pages);
        return -1;
    }

    return new_pages_at_vaddr(vspace, vaddr, num_pages, size_bits, res->rights, res->cacheable, can_use_dev);
}

void *
sel4utils_new_pages(vspace_t *vspace, seL4_CapRights_t rights,
                     size_t num_pages, size_t size_bits)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    int error;
    void *ret_vaddr;

    assert(num_pages > 0);

    ret_vaddr = find_range(data, num_pages, size_bits);
    if (ret_vaddr == NULL) {
        return NULL;
    }

    /* Since sel4utils_new_pages() is an implementation of vspace_new_pages(),
     * it should ideally be preferring to allocate device untypeds and leaving
     * the non-device untypeds for VKA to use when it's allocating kernel objects.
     *
     * Unfortunately it currently has to prefer to allocate non-device untypeds
     * to maintain compatibility with code that uses it incorrectly, such as
     * code that calls vspace_new_pages() to allocate an IPC buffer.
     */
    error = new_pages_at_vaddr(vspace, ret_vaddr, num_pages, size_bits, rights,
                               (int)true, false);
    if (error != 0) {
        if (clear_entries(vspace, (uintptr_t)ret_vaddr, size_bits) != 0) {
            ZF_LOGE("FATAL: Failed to clear VMM metadata for vmem @0x%p, %lu pages.",
                    ret_vaddr, BIT(size_bits));
            /* This is probably cause for a panic, but continue anyway. */
        }
        return NULL;
    }

    return ret_vaddr;
}

int sel4utils_reserve_range_no_alloc_aligned(vspace_t *vspace, sel4utils_res_t *reservation,
                                             size_t size, size_t size_bits, seL4_CapRights_t rights, int cacheable, void **result)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    void *vaddr = find_range(data, BYTES_TO_SIZE_BITS_PAGES(size, size_bits), size_bits);

    if (vaddr == NULL) {
        return -1;
    }

    *result = vaddr;
    reservation->malloced = 0;
    perform_reservation(vspace, reservation, (uintptr_t) vaddr, size, rights, cacheable);
    return 0;
}

int sel4utils_reserve_range_no_alloc(vspace_t *vspace, sel4utils_res_t *reservation, size_t size,
                                     seL4_CapRights_t rights, int cacheable, void **result)
{
    return sel4utils_reserve_range_no_alloc_aligned(vspace, reservation, size, seL4_PageBits,
                                                    rights, cacheable, result);
}

reservation_t
sel4utils_reserve_range_aligned(vspace_t *vspace, size_t bytes, size_t size_bits, seL4_CapRights_t rights,
                                int cacheable, void **result)
{
    reservation_t reservation = {
        .res = NULL,
    };

    if (!sel4_valid_size_bits(size_bits)) {
        ZF_LOGE("Invalid size bits %zu", size_bits);
        return reservation;
    }

    sel4utils_res_t *res = malloc(sizeof(sel4utils_res_t));

    if (res == NULL) {
        ZF_LOGE("Malloc failed");
        return reservation;
    }

    reservation.res = res;

    int error = sel4utils_reserve_range_no_alloc_aligned(vspace, res, bytes, size_bits, rights, cacheable, result);
    if (error) {
        free(reservation.res);
        reservation.res = NULL;
    }

    res->malloced = 1;
    return reservation;
}

int sel4utils_reserve_range_at_no_alloc(vspace_t *vspace, sel4utils_res_t *reservation, void *vaddr,
                                        size_t size, seL4_CapRights_t rights, int cacheable)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    if (!is_available_range(data->top_level, (uintptr_t) vaddr, (uintptr_t)vaddr + size)) {
        ZF_LOGE("Range not available at %p, size %p", vaddr, (void*)size);
        return -1;
    }
    reservation->malloced = 0;
    perform_reservation(vspace, reservation, (uintptr_t) vaddr, size, rights, cacheable);
    return 0;
}

reservation_t
sel4utils_reserve_range_at(vspace_t *vspace, void *vaddr, size_t size, seL4_CapRights_t
                           rights, int cacheable)
{
    reservation_t reservation;
    reservation.res = malloc(sizeof(sel4utils_res_t));

    if (reservation.res == NULL) {
        ZF_LOGE("Malloc failed");
        return reservation;
    }

    int error = sel4utils_reserve_range_at_no_alloc(vspace, reservation.res, vaddr, size, rights, cacheable);

    if (error) {
        free(reservation.res);
        reservation.res = NULL;
    } else {
        ((sel4utils_res_t *)reservation.res)->malloced = 1;
    }

    return reservation;
}

void
sel4utils_free_reservation(vspace_t *vspace, reservation_t reservation)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    sel4utils_res_t *res = reservation.res;

    clear_entries_range(vspace, res->start, res->end, true);
    remove_reservation(data, res);
    if (res->malloced) {
        free(reservation.res);
    }
}

void
sel4utils_free_reservation_by_vaddr(vspace_t *vspace, void *vaddr)
{

    reservation_t reservation;
    reservation.res = find_reserve(get_alloc_data(vspace), (uintptr_t) vaddr);
    sel4utils_free_reservation(vspace, reservation);
}

int
sel4utils_move_resize_reservation(vspace_t *vspace, reservation_t reservation, void *vaddr,
                                  size_t bytes)
{
    assert(reservation.res != NULL);
    sel4utils_res_t *res = reservation.res;
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);

    uintptr_t new_start = ROUND_DOWN((uintptr_t) vaddr, PAGE_SIZE_4K);
    uintptr_t new_end = ROUND_UP(((uintptr_t) (vaddr)) + bytes, PAGE_SIZE_4K);
    uintptr_t v = 0;

    /* Sanity checks that newly asked reservation space is available. */
    if (new_start < res->start) {
        if (!is_available_range(data->top_level, new_start, res->start)) {
            return -1;
        }
    }
    if (new_end > res->end) {
        if (!is_available_range(data->top_level, res->end, new_end)) {
            return -2;
        }
    }

    for (v = new_start; v < new_end; v += PAGE_SIZE_4K) {
        if (v < res->start || v >= res->end) {
            /* Any outside the reservation must be unreserved. */
            int error UNUSED = reserve_entries_range(vspace, v, v + PAGE_SIZE_4K, true);
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
                clear_entries_range(vspace, v, v + PAGE_SIZE_4K, true);
            }
        } else {
            v = new_end - PAGE_SIZE_4K;
        }
    }

    bool need_reinsert = false;
    if (res->start != new_start) {
        need_reinsert = true;
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
    return data->vspace_root;
}

static void free_page(vspace_t *vspace, vka_t *vka, uintptr_t vaddr) {
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    vspace_mid_level_t *level = data->top_level;
    /* see if we should free the thing here or not */
    uintptr_t cookie = get_cookie(level, vaddr);
    int num_4k_entries = 1;
    if (cookie != 0) {
        /* walk along and see just how big this page is */
        uintptr_t test_vaddr = vaddr + PAGE_SIZE_4K;
        while (get_cookie(level, test_vaddr) == cookie) {
            test_vaddr += PAGE_SIZE_4K;
            num_4k_entries++;
        }
        sel4utils_unmap_pages(vspace, (void*)vaddr, 1, PAGE_BITS_4K * num_4k_entries, vka);
    }
}

static void
free_pages_at_level(vspace_t *vspace, vka_t *vka, int table_level, uintptr_t vaddr) {
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    vspace_mid_level_t *level = data->top_level;
    /* walk down to the level that we want */
    for (int i = VSPACE_NUM_LEVELS - 1; i > table_level && i > 1; i--) {
        int index = INDEX_FOR_LEVEL(vaddr, i);
        switch (level->table[index]) {
        case RESERVED:
        case EMPTY:
            return;
        }
        level = (vspace_mid_level_t*)level->table[index];
    }
    if (table_level == 0) {
        int index = INDEX_FOR_LEVEL(vaddr, 1);
        switch (level->table[index]) {
        case RESERVED:
        case EMPTY:
            return;
        }
        vspace_bottom_level_t *bottom = (vspace_bottom_level_t*)level->table[index];
        index = INDEX_FOR_LEVEL(vaddr, 0);
        if (bottom->cap[index] != EMPTY && bottom->cap[index] != RESERVED) {
            free_page(vspace, vka, vaddr);
        }
    } else {
        int index = INDEX_FOR_LEVEL(vaddr, table_level);
        switch (level->table[index]) {
        case RESERVED:
        case EMPTY:
            return;
        }
        /* recurse to the sub level */
        for (int j = 0; j < VSPACE_LEVEL_SIZE; j++) {
            free_pages_at_level(vspace, vka,
                                table_level - 1,
                                vaddr + j * BYTES_FOR_LEVEL(table_level - 1));
        }
        vspace_unmap_pages(data->bootstrap, (void*)level->table[index],
            (table_level == 1 ? sizeof(vspace_bottom_level_t) : sizeof(vspace_mid_level_t)) / PAGE_SIZE_4K, PAGE_BITS_4K, VSPACE_FREE);
    }
}

void
sel4utils_tear_down(vspace_t *vspace, vka_t *vka)
{

    sel4utils_alloc_data_t *data = get_alloc_data(vspace);

    if (data->bootstrap == NULL) {
        ZF_LOGE("Not implemented: sel4utils cannot currently tear down a self-bootstrapped vspace\n");
        return;
    }

    if (vka == VSPACE_FREE) {
        vka = data->vka;
    }

    /* free all the reservations */
    while (data->reservation_head != NULL) {
        reservation_t res = { .res = data->reservation_head };
        sel4utils_free_reservation(vspace, res);
    }

    /* walk each level and find any pages / large pages */
    if (data->top_level) {
        for (int i = 0; i < BIT(VSPACE_LEVEL_BITS); i++) {
            free_pages_at_level(vspace, vka, VSPACE_NUM_LEVELS - 1, BYTES_FOR_LEVEL(VSPACE_NUM_LEVELS - 1) * i);
        }
        vspace_unmap_pages(data->bootstrap, data->top_level, sizeof(vspace_mid_level_t) / PAGE_SIZE_4K, PAGE_BITS_4K, VSPACE_FREE);
    }
}

int
sel4utils_share_mem_at_vaddr(vspace_t *from, vspace_t *to, void *start, int num_pages,
                             size_t size_bits, void *vaddr, reservation_t reservation)
{
    int error = 0; /* no error */
    sel4utils_alloc_data_t *from_data = get_alloc_data(from);
    sel4utils_alloc_data_t *to_data = get_alloc_data(to);
    cspacepath_t from_path, to_path;
    int page;
    sel4utils_res_t *res = reservation_to_res(reservation);

    if (!sel4_valid_size_bits(size_bits)) {
        ZF_LOGE("Invalid size bits %zu", size_bits);
        return -1;
    }

    /* go through, page by page, and duplicate the page cap into the to cspace and
     * map it into the to vspace */
    size_t size_bytes = 1 << size_bits;
    for (page = 0; page < num_pages; page++) {
        uintptr_t from_vaddr = (uintptr_t) start + page * size_bytes;
        uintptr_t to_vaddr = (uintptr_t) vaddr + (uintptr_t) page * size_bytes;

        /* get the frame cap to be copied */
        seL4_CPtr cap = get_cap(from_data->top_level, from_vaddr);
        if (cap == seL4_CapNull) {
            ZF_LOGE("Cap not present in from vspace to copy, vaddr %"PRIuPTR, from_vaddr);
            error = -1;
            break;
        }

        /* create a path to the cap */
        vka_cspace_make_path(from_data->vka, cap, &from_path);

        /* allocate a path to put the copy in the destination */
        error = vka_cspace_alloc_path(to_data->vka, &to_path);
        if (error) {
            ZF_LOGE("Failed to allocate slot in to cspace, error: %d", error);
            break;
        }

        /* copy the frame cap into the to cspace */
        error = vka_cnode_copy(&to_path, &from_path, res->rights);
        if (error) {
            ZF_LOGE("Failed to copy cap, error %d\n", error);
            break;
        }

        /* now finally map the page */
        error = map_page(to, to_path.capPtr, (void *) to_vaddr, res->rights, res->cacheable, size_bits);
        if (error) {
            ZF_LOGE("Failed to map page into target vspace at vaddr %"PRIuPTR, to_vaddr);
            break;
        }

        update_entries(to, to_vaddr, to_path.capPtr, size_bits, 0);
    }

    if (error) {
        /* we didn't finish, undo any pages we did map */
        vspace_unmap_pages(to, vaddr, page, size_bits, VSPACE_FREE);
    }

    return error;
}

uintptr_t
sel4utils_get_paddr(vspace_t *vspace, void *vaddr, seL4_Word type, seL4_Word size_bits)
{
    vka_t *vka = get_alloc_data(vspace)->vka;
    return vka_utspace_paddr(vka, vspace_get_cookie(vspace, vaddr), type, size_bits);

}

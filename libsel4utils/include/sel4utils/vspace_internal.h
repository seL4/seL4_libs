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

#pragma once

#include <autoconf.h>
#include <vka/vka.h>
#include <vspace/vspace.h>

#include <sel4utils/util.h>
#include <sel4utils/mapping.h>
#include <sel4utils/vspace.h>

#define RESERVED UINTPTR_MAX
#define EMPTY    0

#define TOP_LEVEL_BITS_OFFSET (VSPACE_LEVEL_BITS * (VSPACE_NUM_LEVELS - 1) + PAGE_BITS_4K)
#define LEVEL_MASK MASK_UNSAFE(VSPACE_LEVEL_BITS)

#define INDEX_FOR_LEVEL(addr, l) ( ( (addr) >> ((l) * VSPACE_LEVEL_BITS + PAGE_BITS_4K)) & MASK(VSPACE_LEVEL_BITS))
#define TOP_LEVEL_INDEX(x) INDEX_FOR_LEVEL(x, VSPACE_NUM_LEVELS - 1)

#define BYTES_FOR_LEVEL(l) BIT(VSPACE_LEVEL_BITS * (l) + PAGE_BITS_4K)
#define ALIGN_FOR_LEVEL(l) (~(MASK(VSPACE_LEVEL_BITS * (l) + PAGE_BITS_4K)))

void *create_level(vspace_t *vspace, size_t size);
void *bootstrap_create_level(vspace_t *vspace, size_t size);

static inline void *create_mid_level(vspace_t *vspace, uintptr_t init) {
    vspace_mid_level_t *level = create_level(vspace, sizeof(vspace_mid_level_t));
    if (level) {
        for (int i = 0; i < VSPACE_LEVEL_SIZE; i++) {
            level->table[i] = init;
        }
    }
    return level;
}

static inline void *create_bottom_level(vspace_t *vspace, uintptr_t init) {
    vspace_bottom_level_t *level = create_level(vspace, sizeof(vspace_bottom_level_t));
    if (level) {
        for (int i = 0; i < VSPACE_LEVEL_SIZE; i++) {
            level->cap[i] = init;
            level->cookie[i] = 0;
        }
    }
    return level;
}

static inline sel4utils_alloc_data_t *
get_alloc_data(vspace_t *vspace)
{
    return (sel4utils_alloc_data_t *) vspace->data;
}

static int
reserve_entries_bottom(vspace_t *vspace, vspace_bottom_level_t *level, uintptr_t start, uintptr_t end, bool preserve_frames)
{
    while (start < end) {
        int index = INDEX_FOR_LEVEL(start, 0);
        uintptr_t cap = level->cap[index];
        if (cap == RESERVED) {
            ZF_LOGE("Attempting to reserve already reserved region");
            return -1;
        }
        if (cap != EMPTY && preserve_frames) {
            return -1;
        }
        level->cap[index] = RESERVED;
        level->cookie[index] = 0;
        start += BYTES_FOR_LEVEL(0);
    }
    return 0;
}

static int
reserve_entries_mid(vspace_t *vspace, vspace_mid_level_t *level, int level_num, uintptr_t start, uintptr_t end, bool preserve_frames)
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
        if (next_table == RESERVED) {
            ZF_LOGE("Tried to reserve already reserved region");
            return -1;
        }
        if (next_table == EMPTY) {
            if (must_recurse) {
                /* allocate new level */
                if (level_num == 1) {
                    next_table = (uintptr_t)create_bottom_level(vspace, EMPTY);
                } else {
                    next_table = (uintptr_t)create_mid_level(vspace, EMPTY);
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
                error = reserve_entries_bottom(vspace, (vspace_bottom_level_t*)next_table, start, next_start, preserve_frames);
            } else {
                error = reserve_entries_mid(vspace, (vspace_mid_level_t*)next_table, level_num - 1, start, next_start, preserve_frames);
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
clear_entries_bottom(vspace_t *vspace, vspace_bottom_level_t *level, uintptr_t start, uintptr_t end, bool only_reserved)
{
    while (start < end) {
        int index = INDEX_FOR_LEVEL(start, 0);
        uintptr_t cap = level->cap[index];
        if (cap != RESERVED && only_reserved) {
            return -1;
        }
        level->cap[index] = EMPTY;
        level->cookie[index] = 0;
        start += BYTES_FOR_LEVEL(0);
    }
    return 0;
}

static int
clear_entries_mid(vspace_t *vspace, vspace_mid_level_t *level, int level_num, uintptr_t start, uintptr_t end, bool only_reserved)
{
    /* walk entries at this level until we complete this range */
    while (start < end) {
        int index = INDEX_FOR_LEVEL(start, level_num);
        /* align the start so we can check for alignment later */
        uintptr_t aligned_start = start & ALIGN_FOR_LEVEL(level_num);
        /* calculate the start of the next index */
        uintptr_t next_start = aligned_start + BYTES_FOR_LEVEL(level_num);
        if (next_start > end) {
            next_start = end;
        }
        uintptr_t next_table = level->table[index];
        if (next_table == RESERVED) {
            ZF_LOGE("Cannot clear reserved entries mid level");
            return -1;
        }
        if (next_table != EMPTY) {
            int error;
            if (level_num == 1) {
                error = clear_entries_bottom(vspace, (vspace_bottom_level_t*)next_table, start, next_start, only_reserved);
            } else {
                error = clear_entries_mid(vspace, (vspace_mid_level_t*)next_table, level_num - 1, start, next_start, only_reserved);
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
update_entries_bottom(vspace_t *vspace, vspace_bottom_level_t *level, uintptr_t start, uintptr_t end, seL4_CPtr cap, uintptr_t cookie)
{
    while (start < end) {
        int index = INDEX_FOR_LEVEL(start, 0);
        uintptr_t old_cap = level->cap[index];
        if (old_cap != RESERVED && old_cap != EMPTY) {
            ZF_LOGE("Mapping neither reserved nor empty for vaddr %" PRIxPTR " (contains 0x%" PRIxPTR ")", start, old_cap);
            return -1;
        }
        level->cap[index] = cap;
        level->cookie[index] = cookie;
        start += BYTES_FOR_LEVEL(0);
    }
    return 0;
}

static int
update_entries_mid(vspace_t *vspace, vspace_mid_level_t *level, int level_num, uintptr_t start, uintptr_t end, seL4_CPtr cap, uintptr_t cookie)
{
    /* walk entries at this level until we complete this range */
    while (start < end) {
        int index = INDEX_FOR_LEVEL(start, level_num);
        /* align the start so we can check for alignment later */
        uintptr_t aligned_start = start & ALIGN_FOR_LEVEL(level_num);
        /* calculate the start of the next index */
        uintptr_t next_start = aligned_start + BYTES_FOR_LEVEL(level_num);
        if (next_start > end) {
            next_start = end;
        }
        uintptr_t next_table = level->table[index];
        if (next_table == EMPTY || next_table == RESERVED) {
            /* allocate new level */
            if (level_num == 1) {
                next_table = (uintptr_t)create_bottom_level(vspace, next_table);
            } else {
                next_table = (uintptr_t)create_mid_level(vspace, next_table);
            }
            if (next_table == EMPTY) {
                ZF_LOGE("Failed to allocate and map book keeping frames during bootstrapping");
                return -1;
            }
            level->table[index] = next_table;
        }
        int error;
        if (level_num == 1) {
            error = update_entries_bottom(vspace, (vspace_bottom_level_t*)next_table, start, next_start, cap, cookie);
        } else {
            error = update_entries_mid(vspace, (vspace_mid_level_t*)next_table, level_num - 1, start, next_start, cap, cookie);
        }
        if (error) {
            return error;
        }
        start = next_start;
    }
    return 0;
}

static bool
is_reserved_or_empty_bottom(vspace_bottom_level_t *level, uintptr_t start, uintptr_t end, uintptr_t good, uintptr_t bad)
{
    while (start < end) {
        int index = INDEX_FOR_LEVEL(start, 0);
        uintptr_t cap = level->cap[index];
        if (cap != good) {
            return false;
        }
        start += BYTES_FOR_LEVEL(0);
    }
    return true;
}

static bool
is_reserved_or_empty_mid(vspace_mid_level_t *level, int level_num, uintptr_t start, uintptr_t end, uintptr_t good, uintptr_t bad)
{
    /* walk entries at this level until we complete this range */
    while (start < end) {
        int index = INDEX_FOR_LEVEL(start, level_num);
        /* align the start so we can check for alignment later */
        uintptr_t aligned_start = start & ALIGN_FOR_LEVEL(level_num);
        /* calculate the start of the next index */
        uintptr_t next_start = aligned_start + BYTES_FOR_LEVEL(level_num);
        if (next_start > end) {
            next_start = end;
        }
        uintptr_t next_table = level->table[index];
        if (next_table == bad) {
            return false;
        }
        if (next_table != good) {
            int succ;
            if (level_num == 1) {
                succ = is_reserved_or_empty_bottom((vspace_bottom_level_t*)next_table, start, next_start, good, bad);
            } else {
                succ = is_reserved_or_empty_mid((vspace_mid_level_t*)next_table, level_num - 1, start, next_start, good, bad);
            }
            if (!succ) {
                return false;
            }
        }
        start = next_start;
    }
    return true;
}

/* update entry in page table and handle large pages */
static inline int
update_entries(vspace_t *vspace, uintptr_t vaddr, seL4_CPtr cap, size_t size_bits, uintptr_t cookie)
{
    uintptr_t start = vaddr;
    uintptr_t end = vaddr + BIT(size_bits);
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    return update_entries_mid(vspace, data->top_level, VSPACE_NUM_LEVELS - 1, start, end, cap, cookie);
}

static inline int
reserve_entries_range(vspace_t *vspace, uintptr_t start, uintptr_t end, bool preserve_frames)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    return reserve_entries_mid(vspace, data->top_level, VSPACE_NUM_LEVELS - 1, start, end, preserve_frames);
}

static inline int
reserve_entries(vspace_t *vspace, uintptr_t vaddr, size_t size_bits)
{
    uintptr_t start = vaddr;
    uintptr_t end = vaddr + BIT(size_bits);
    return reserve_entries_range(vspace, start, end, false);
}

static inline int
clear_entries_range(vspace_t *vspace, uintptr_t start, uintptr_t end, bool only_reserved)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    int error = clear_entries_mid(vspace, data->top_level, VSPACE_NUM_LEVELS - 1, start, end, only_reserved);
    if (error) {
        return error;
    }

    if (start < data->last_allocated) {
        data->last_allocated = start;
    }

    return 0;
}

static inline int
clear_entries(vspace_t *vspace, uintptr_t vaddr, size_t size_bits)
{
    uintptr_t start = vaddr;
    uintptr_t end = vaddr + BIT(size_bits);
    return clear_entries_range(vspace, start, end, false);
}

static inline bool
is_reserved_or_empty_range(vspace_mid_level_t *top_level, uintptr_t start, uintptr_t end, uintptr_t good, uintptr_t bad)
{
    return is_reserved_or_empty_mid(top_level, VSPACE_NUM_LEVELS - 1, start, end, good, bad);
}

static inline bool
is_reserved_or_empty(vspace_mid_level_t *top_level, uintptr_t vaddr, size_t size_bits, uintptr_t good, uintptr_t bad)
{
    uintptr_t start = vaddr;
    uintptr_t end = vaddr + BIT(size_bits);
    return is_reserved_or_empty_range(top_level, start, end, good, bad);
}

static inline bool
is_available_range(vspace_mid_level_t *top_level, uintptr_t start, uintptr_t end)
{
    return is_reserved_or_empty_range(top_level, start, end, EMPTY, RESERVED);
}

static inline bool
is_available(vspace_mid_level_t *top_level, uintptr_t vaddr, size_t size_bits)
{
    return is_reserved_or_empty(top_level, vaddr, size_bits, EMPTY, RESERVED);
}

static inline bool
is_reserved_range(vspace_mid_level_t *top_level, uintptr_t start, uintptr_t end)
{
    return is_reserved_or_empty_range(top_level, start, end, RESERVED, EMPTY);
}

static inline bool
is_reserved(vspace_mid_level_t *top_level, uintptr_t vaddr, size_t size_bits)
{
    return is_reserved_or_empty(top_level, vaddr, size_bits, RESERVED, EMPTY);
}

static inline seL4_CPtr
get_cap(vspace_mid_level_t *top, uintptr_t vaddr)
{
    vspace_mid_level_t *level = top;
    for (int i = VSPACE_NUM_LEVELS - 1; i > 1; i--) {
        int index = INDEX_FOR_LEVEL(vaddr, i);
        uintptr_t next = level->table[index];
        if (next == EMPTY || next == RESERVED) {
            return 0;
        }
        level = (vspace_mid_level_t*)next;
    }
    uintptr_t next = level->table[INDEX_FOR_LEVEL(vaddr, 1)];
    if (next == EMPTY || next == RESERVED) {
        return 0;
    }
    vspace_bottom_level_t *bottom = (vspace_bottom_level_t*)next;
    return bottom->cap[INDEX_FOR_LEVEL(vaddr, 0)];
}

static inline uintptr_t
get_cookie(vspace_mid_level_t *top, uintptr_t vaddr)
{
    vspace_mid_level_t *level = top;
    for (int i = VSPACE_NUM_LEVELS - 1; i > 1; i--) {
        int index = INDEX_FOR_LEVEL(vaddr, i);
        uintptr_t next = level->table[index];
        if (next == EMPTY || next == RESERVED) {
            return 0;
        }
        level = (vspace_mid_level_t*)next;
    }
    uintptr_t next = level->table[INDEX_FOR_LEVEL(vaddr, 1)];
    if (next == EMPTY || next == RESERVED) {
        return 0;
    }
    vspace_bottom_level_t *bottom = (vspace_bottom_level_t*)next;
    return bottom->cookie[INDEX_FOR_LEVEL(vaddr, 0)];
}

/* Internal interface functions */
int sel4utils_map_page_pd(vspace_t *vspace, seL4_CPtr cap, void *vaddr, seL4_CapRights_t rights, int cacheable, size_t size_bits);
#ifdef CONFIG_VTX
int sel4utils_map_page_ept(vspace_t *vspace, seL4_CPtr cap, void *vaddr, seL4_CapRights_t rights, int cacheable, size_t size_bits);
#endif /* CONFIG_VTX */
#ifdef CONFIG_IOMMU
int sel4utils_map_page_iommu(vspace_t *vspace, seL4_CPtr cap, void *vaddr, seL4_CapRights_t rights, int cacheable, size_t size_bits);
#endif /* CONFIG_IOMMU */

/* interface functions */
seL4_CPtr sel4utils_get_root(vspace_t *vspace);
seL4_CPtr sel4utils_get_cap(vspace_t *vspace, void *vaddr);
uintptr_t sel4utils_get_cookie(vspace_t *vspace, void *vaddr);

void *sel4utils_map_pages(vspace_t *vspace, seL4_CPtr caps[], uintptr_t cookies[],
                          seL4_CapRights_t rights, size_t num_pages, size_t size_bits,
                          int cacheable);
int sel4utils_map_pages_at_vaddr(vspace_t *vspace, seL4_CPtr caps[], uintptr_t cookies[], void *vaddr,
                                 size_t num_pages, size_t size_bits, reservation_t reservation);
void sel4utils_unmap_pages(vspace_t *vspace, void *vaddr, size_t num_pages, size_t size_bits, vka_t *vka);

void *sel4utils_new_pages(vspace_t *vspace, seL4_CapRights_t rights,
                          size_t num_pages, size_t size_bits);
int sel4utils_new_pages_at_vaddr(vspace_t *vspace, void *vaddr, size_t num_pages,
                                 size_t size_bits, reservation_t reservation, bool can_use_dev);

reservation_t sel4utils_reserve_range_aligned(vspace_t *vspace, size_t size, size_t size_bits, seL4_CapRights_t rights,
                                              int cacheable, void **vaddr);
reservation_t sel4utils_reserve_range_at(vspace_t *vspace, void *vaddr, size_t size,
                                         seL4_CapRights_t rights, int cacheable);
void sel4utils_free_reservation(vspace_t *vspace, reservation_t reservation);
void sel4utils_free_reservation_by_vaddr(vspace_t *vspace, void *vaddr);
void sel4utils_tear_down(vspace_t *vspace, vka_t *vka);
int sel4utils_share_mem_at_vaddr(vspace_t *from, vspace_t *to, void *start, int num_pages,
                                 size_t size_bits, void *vaddr, reservation_t reservation);


/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __SEL4UILS_VSPACE_PRIVATE_H
#define __SEL4UILS_VSPACE_PRIVATE_H

#include <vka/vka.h>
#include <vspace/vspace.h>

#include <sel4utils/util.h>
#include <sel4utils/mapping.h>
#include <sel4utils/vspace.h>

#define KERNEL_RESERVED_START 0xE0000000
#define KERNEL_RESERVED_END   0xFFFFF000

#define RESERVED 0xFFFFFFFF
#define EMPTY    0

/* Bits to remove to get to the start of the second level translation bits. There are
 * many implicit assumptions in the code that this is actually 12 bits and hence
 * translation addresses can be stepped using the PAGE_SIZE_4K constant. */
#ifdef CONFIG_X86_64
#define BOTTOM_LEVEL_BITS_OFFSET (30 - 2 * VSPACE_LEVEL_BITS)
#else
#define BOTTOM_LEVEL_BITS_OFFSET (seL4_WordBits - 2 * VSPACE_LEVEL_BITS)
#endif

/* Bits to remove to get to the start of the first level translation bits */
#ifdef CONFIG_X86_64
#define TOP_LEVEL_BITS_OFFSET (30 - VSPACE_LEVEL_BITS)
#else
#define TOP_LEVEL_BITS_OFFSET (seL4_WordBits - VSPACE_LEVEL_BITS)
#endif

#define LEVEL_MASK MASK_UNSAFE(VSPACE_LEVEL_BITS)

#define BOTTOM_LEVEL_INDEX(x) ((((uint32_t) ((seL4_Word)(x))) >> BOTTOM_LEVEL_BITS_OFFSET) & LEVEL_MASK)
#define TOP_LEVEL_INDEX(x) ((((uint32_t) ((seL4_Word)(x))) >> TOP_LEVEL_BITS_OFFSET)  & LEVEL_MASK)

int assure_level(vspace_t *vspace, void *vaddr);
int bootstrap_create_level(vspace_t *vspace, void *vaddr);

static inline sel4utils_alloc_data_t *
get_alloc_data(vspace_t *vspace)
{
    return (sel4utils_alloc_data_t *) vspace->data;
}

static inline int
update_entry(vspace_t *vspace, void *vaddr, seL4_CPtr page)
{
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);

    if (assure_level(vspace, vaddr) != seL4_NoError) {
        return -1;
    }

    assert(data->top_level[TOP_LEVEL_INDEX(vaddr)] != (void *) RESERVED);
    assert(data->top_level[TOP_LEVEL_INDEX(vaddr)] != NULL);
    data->top_level[TOP_LEVEL_INDEX(vaddr)]->bottom_level[BOTTOM_LEVEL_INDEX(vaddr)] = page;

    return seL4_NoError;
}

static inline int
update(vspace_t *vspace, void *vaddr, seL4_CPtr page)
{
   
#if 0
    if (page == RESERVED) {
        LOG_ERROR("Cap %x cannot be used!", RESERVED);
        return -1;
    }
#endif 
    assert(page != 0);
    assert(get_alloc_data(vspace)->top_level[TOP_LEVEL_INDEX(vaddr)] != (void *) RESERVED);
    return update_entry(vspace, vaddr, page);
}

static inline int
reserve(vspace_t *vspace, void *vaddr)
{
    assert(get_alloc_data(vspace)->top_level[TOP_LEVEL_INDEX(vaddr)] != (void *) RESERVED);
    return update_entry(vspace, vaddr, RESERVED);
}

static inline void
clear(vspace_t *vspace, void *vaddr)
{
    /* if we are clearing an entry it should have been allocated before */
    sel4utils_alloc_data_t *data = get_alloc_data(vspace);
    assert(data->top_level[TOP_LEVEL_INDEX(vaddr)] != NULL);
    assert(data->top_level[TOP_LEVEL_INDEX(vaddr)] != (void *) RESERVED);

    update_entry(vspace, vaddr, 0);
    if (vaddr < data->last_allocated) {
        data->last_allocated = vaddr;
    }
}

static inline seL4_CPtr
get_entry(bottom_level_t *top[], void *vaddr)
{
    if (top[TOP_LEVEL_INDEX(vaddr)] == NULL) {
        return 0;
    }

    return top[TOP_LEVEL_INDEX(vaddr)]->bottom_level[BOTTOM_LEVEL_INDEX(vaddr)];
}

/* update entry in page table and handle large pages */
static inline int
update_entries(vspace_t *vspace, void *vaddr, seL4_CPtr cap, size_t size_bits)
{
    int error = seL4_NoError;
    for (int i = 0; i < (1 << size_bits) && error == seL4_NoError; i += PAGE_SIZE_4K) {
        error = update(vspace, vaddr + i, cap);
    }

    return error;
}

/* reserve entry in page table and handle large pages */
static inline int
reserve_entries(vspace_t *vspace, void *vaddr, size_t size_bits)
{
    int error = seL4_NoError;
    for (int i = 0; i < (1 << size_bits) && error == seL4_NoError; i += PAGE_SIZE_4K) {
        error = reserve(vspace, vaddr + i);
    }

    return error;
}

/* clear an entry in the page table and handle large pages */
static inline void
clear_entries(vspace_t *vspace, void *vaddr, size_t size_bits)
{
    assert( ((uintptr_t)vaddr & ((1 << size_bits) - 1)) == 0);
    for (int i = 0; i < (1 << size_bits); i += PAGE_SIZE_4K) {
        clear(vspace, vaddr + i);
    }
}

static inline int
is_available(bottom_level_t *top_level[VSPACE_LEVEL_SIZE], void *vaddr)
{
    /* if there is no bottom level page table then this entry is available */
    if (top_level[TOP_LEVEL_INDEX(vaddr)] == NULL) {
        return 1;
        /* otherwise check the entry explicitly */
    } else if (top_level[TOP_LEVEL_INDEX(vaddr)] == (void *) RESERVED) {
        return 0;
    }

    return top_level[TOP_LEVEL_INDEX(vaddr)]->bottom_level[BOTTOM_LEVEL_INDEX(vaddr)] == 0;
}

static inline int
is_reserved(bottom_level_t *top_level[VSPACE_LEVEL_SIZE], void *vaddr)
{
    if (top_level[TOP_LEVEL_INDEX(vaddr)] == (void *)  RESERVED) {
        return 1;
    }

    if (top_level[TOP_LEVEL_INDEX(vaddr)] == NULL) {
        return 0;
    }
#if 0
    printf("is reserved vaddr 0x%x 0x%x \n",vaddr, top_level[TOP_LEVEL_INDEX(vaddr)]->bottom_level[BOTTOM_LEVEL_INDEX(vaddr)]);
#endif
    return top_level[TOP_LEVEL_INDEX(vaddr)]->bottom_level[BOTTOM_LEVEL_INDEX(vaddr)] == RESERVED;
}

/* Internal interface functions */
int sel4utils_map_page_pd(vspace_t *vspace, seL4_CPtr cap, void *vaddr, seL4_CapRights rights, int cacheable, size_t size_bits);
#ifdef CONFIG_VTX
int sel4utils_map_page_ept(vspace_t *vspace, seL4_CPtr cap, void *vaddr, seL4_CapRights rights, int cacheable, size_t size_bits);
#endif /* CONFIG_VTX */

/* interface functions */
int sel4utils_map_pages_at_vaddr(vspace_t *vspace, seL4_CPtr caps[], void *vaddr,
        size_t num_pages, size_t size_bits, reservation_t *reservation);
seL4_CPtr sel4utils_get_cap(vspace_t *vspace, void *vaddr);
void *sel4utils_map_pages(vspace_t *vspace, seL4_CPtr caps[], seL4_CapRights rights,
        size_t num_pages, size_t size_bits, int cacheable);
void sel4utils_unmap_pages(vspace_t *vspace, void *vaddr, size_t num_pages, size_t size_bits);

int sel4utils_unmap_reserved_pages(vspace_t *vspace, void *vaddr, size_t num_pages, size_t size_bits, reservation_t *reservation);

int sel4utils_new_pages_at_vaddr(vspace_t *vspace, void *vaddr, size_t num_pages,
        size_t size_bits, reservation_t *reservation);
void *sel4utils_new_pages(vspace_t *vspace, seL4_CapRights rights, size_t num_pages,
        size_t size_bits);
void sel4utils_free_pages(vspace_t *vspace, void *vaddr, size_t num_pages, size_t size_bits);

void *sel4utils_new_stack(vspace_t *vspace);
void sel4utils_free_stack(vspace_t *vspace, void *stack_top);

void * sel4utils_new_ipc_buffer(vspace_t *vspace, seL4_CPtr *page);
void sel4utils_free_ipc_buffer(vspace_t *vspace, void *vaddr);

reservation_t *sel4utils_reserve_range(vspace_t *vspace, size_t size, seL4_CapRights rights,
        int cacheable, void **vaddr);
reservation_t *sel4utils_reserve_range_at(vspace_t *vspace, void *vaddr, size_t size,
        seL4_CapRights rights, int cacheable);
void sel4utils_free_reservation(vspace_t *vspace, reservation_t *reservation);

#endif /* __SEL4UILS_VSPACE_PRIVATE_H */

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _INTERFACE_VSPACE_H_
#define _INTERFACE_VSPACE_H_

#include <assert.h>
#include <stddef.h>
#include <vka/object.h>

typedef struct vspace vspace_t;

typedef struct reservation reservation_t;

/**
 * Create a stack. The determines stack size.
 *
 * @param vspace the virtual memory allocator used.
 *
 * @return virtual address of the top of the created stack.
 *         NULL on failure.
 */
typedef void *(*vspace_new_stack_fn)(vspace_t *vspace);

/**
 * Free a stack. This will only free virtual resources, not physical resources.
 *
 * @param vspace the virtual memory allocator used.
 * @param stack_top the top of the stack.
 *
 */
typedef void (*vspace_free_stack_fn)(vspace_t *vspace, void *stack_top);

/**
 * Create an IPC buffer.
 *
 * @param vspace the virtual memory allocator used.
 * @param page capability to that the IPC buffer was mapped in with
 *
 * @return vaddr of the mapped in IPC buffer
 *         NULL on failure.
 */
typedef void *(*vspace_new_ipc_buffer_fn)(vspace_t *vspace, seL4_CPtr *page);

/**
 * Free an IPC buffer. This will only free virtual resources, not physical resources.
 *
 *
 * @param vspace the virtual memory allocator used.
 * @param addr address the IPC buffer was mapped in to.
 *
 */
typedef void (*vspace_free_ipc_buffer_fn)(vspace_t *vspace, void *addr);

/**
 * Create a virtually contiguous area of mapped pages.
 * This could be for shared memory or just allocating some pages.
 *
 * @param vspace the virtual memory allocator used.
 * @param rights the rights to map the pages in with
 * @param num_pages the number of pages to allocate and map.
 * @param size_bits size of the pages to allocate and map, in bits.
 *
 * @return vaddr at the start of the contiguous region
 *         NULL on failure.
 */
typedef void *(*vspace_new_pages_fn)(vspace_t *vspace, seL4_CapRights rights,
                                          size_t num_pages, size_t size_bits);

/**
 * Create a virtually contiguous area of mapped pages, at the specified virtual address.
 *
 * This is designed for elf loading, where virtual addresses are chosen for you.
 * The vspace allocator will not allow this address to be reused unless you free it.
 *
 * This function will FAIL if the virtual address range requested is not free.
 *
 *
 * @param vspace the virtual memory allocator used.
 * @param vaddr the virtual address to start allocation at.
 * @param num_pages the number of pages to allocate and map.
 * @param size_bits size of the pages to allocate and map, in bits.
 * @param reservation reservation to the range the allocation will take place in.
 *
 * @return seL4_NoError on success, -1 otherwise.
 */
typedef int (*vspace_new_pages_at_vaddr_fn)(vspace_t *vspace, void *vaddr, size_t num_pages,
                                            size_t size_bits, reservation_t *reservation);

/**
 * Free a virtually contiguous area of mapped pages.
 * This does not free physical resources.
 *
 * @param vspace the virtual memory allocator used.
 * @param vaddr vaddr that the pages were mapped in to.
 * @param num_pages the number of pages to free.
 * @param size_bits size of the pages, in bits.
 */
typedef void (*vspace_free_pages_fn)(vspace_t *vspace, void *vaddr,
                                          size_t num_pages, size_t size_bits);

/**
 * Map in existing page capabilities, using contiguos virtual memory.
 *
 * @param vspace the virtual memory allocator used.
 * @param seL4_CPtr caps array of caps to map in
 * @param rights the rights to map the pages in with
 * @param size_bits size, in bits, of an individual page -- all pages must be the same size.
 * @param num_pages the number of pages to map in (must correspond to the size of the array).
 * @param cacheable 1 if the pages should be mapped with cacheable attributes. 0 for DMA.
 *
 * @return vaddr at the start of the device mapping
 *         NULL on failure.
 */
typedef void *(*vspace_map_pages_fn)(vspace_t *vspace, seL4_CPtr caps[],
                                          seL4_CapRights rights, size_t num_pages, size_t size_bits,
                                          int cacheable);

/**
 * Map in existing page capabilities, using contiguos virtual memory at the specified virtual address.
 *
 * This will FAIL if the virtual address is already mapped in.
 *
 * @param vspace the virtual memory allocator used.
 * @param seL4_CPtr caps array of caps to map in
 * @param size_bits size, in bits, of an individual page -- all pages must be the same size.
 * @param num_pages the number of pages to map in (must correspond to the size of the array).
 * @param reservation reservation to the range the allocation will take place in.
 *
 * @return seL4_NoError on success. -1 on failure.
 *         NULL on failure.
 */
typedef int (*vspace_map_pages_at_vaddr_fn)(vspace_t *vspace, seL4_CPtr caps[],
                                          void *vaddr, size_t num_pages,
                                          size_t size_bits, reservation_t *reservation);

/**
 * Unmap in existing page capabilities that use contiguos virtual memory.
 * This function will not free the virtual memory.
 * Unmapped memory is marked as free, even if a reservation exists
 *
 * @param vspace the virtual memory allocator used.
 * @param vaddr the start of the contiguous region.
 * @param size_bits size, in bits, of an individual page -- all pages must be the same size.
 * @param num_pages the number of pages to map in (must correspond to the size of the array).
 *
 */
typedef void (*vspace_unmap_pages_fn)(vspace_t *vspace, void *vaddr, size_t num_pages,
        size_t size_bits);

/**
 * Unmap in existing page capabilities that use contiguos virtual memory.
 * This function will not free the virtual memory.
 * This function will update unmapped area as reserved, provided the correct
 * reservation is passed in
 *
 * @param vspace the virtual memory allocator used.
 * @param vaddr the start of the contiguous region.
 * @param size_bits size, in bits, of an individual page -- all pages must be the same size.
 * @param num_pages the number of pages to map in (must correspond to the size of the array).
 *
 * @return Returns 0 on success
 */
typedef int (*vspace_unmap_reserved_pages_fn)(vspace_t *vspace, void *vaddr, size_t num_pages,
        size_t size_bits, reservation_t *reservation);

/**
 * Reserve a range to map memory into later.
 * Regions will be aligned to 4K boundaries.
 *
 * @param vspace the virtual memory allocator to use.
 * @param bytes the size in bytes to map.
 * @param rights the rights to map the pages in with in this reservation
 * @param cacheable 1 if the pages should be mapped with cacheable attributes. 0 for DMA.
 * @param vaddr the virtual address of the reserved range will be returned here.
 *
 * @return a reservation to use when mapping pages in the range.
 */
typedef reservation_t *(*vspace_reserve_range_fn)(vspace_t *vspace, size_t bytes,
        seL4_CapRights rights, int cacheable, void **vaddr);

/**
 * Reserve a range to map memory in to later at a specific address.
 * Regions will be aligned to 4K boundaries.
 *
 * @param vspace the virtual memory allocator to use.
 * @param vaddr the virtual address to start the range at.
 * @param bytes the size in bytes to map.
 * @param rights the rights to map the pages in with in this reservation
 * @param cacheable 1 if the pages should be mapped with cacheable attributes. 0 for DMA.
 *
 * @return a reservation to use when mapping pages in the range.
 */
typedef reservation_t *(*vspace_reserve_range_at_fn)(vspace_t *vspace, void *vaddr,
        size_t bytes, seL4_CapRights rights, int cacheable);

/**
 * Free a reservation.
 *
 * This will not touch any pages, but will unreserve any reserved addresses in the reservation.
 *
 * @param vspace the virtual memory allocator to use.
 * @param reservation the reservation to free.
 */
typedef void (*vspace_free_reservation_fn)(vspace_t *vspace, reservation_t *reservation);

/**
 * Get the capability mapped at a virtual address.
 *
 *
 * @param vspace the virtual memory allocator to use.
 * @param vaddr the virtual address to get the cap for.
 *
 * @return the cap mapped to this virtual address, 0 otherwise.
 */
 typedef seL4_CPtr (*vspace_get_cap_fn)(vspace_t *vspace, void *vaddr);

/**
 * Function that the vspace allocator will call if it allocates any memory.
 * This allows the user to clean up the allocation at a later time.
 *
 * If this function is null, it will not be called.
 *
 * @param vspace the virtual memory space allocator to use.
 * @param allocated_object_cookie A cookie provided by the user when the vspace allocator is
 *                                initialised --> vspace->allocated_object_cookie/
 * @param object the object that was allocated.
 */
typedef void (*vspace_allocated_object_fn)(void *allocated_object_cookie, vka_object_t object);

/* Portable virtual memory allocation interface */
struct vspace {
    void *data;
    vspace_new_stack_fn new_stack;
    vspace_free_stack_fn free_stack;

    vspace_new_ipc_buffer_fn new_ipc_buffer;
    vspace_free_ipc_buffer_fn free_ipc_buffer;

    vspace_new_pages_fn new_pages;
    vspace_new_pages_at_vaddr_fn new_pages_at_vaddr;
    vspace_free_pages_fn free_pages;

    vspace_map_pages_fn map_pages;
    vspace_map_pages_at_vaddr_fn map_pages_at_vaddr;
    vspace_unmap_pages_fn unmap_pages;
    vspace_unmap_reserved_pages_fn unmap_reserved_pages;

    vspace_reserve_range_fn reserve_range;
    vspace_reserve_range_at_fn reserve_range_at;
    vspace_free_reservation_fn free_reservation;

    vspace_get_cap_fn get_cap;

    seL4_CPtr page_directory;

    vspace_allocated_object_fn allocated_object;
    void *allocated_object_cookie;
};

/* convenient wrappers */

static inline void * 
vspace_new_stack(vspace_t *vspace)
{
    assert(vspace);
    assert(vspace->new_stack);
    return vspace->new_stack(vspace);
}

static inline void
vspace_free_stack(vspace_t *vspace, void *stack_top)
{
    assert(vspace);
    assert(vspace->free_stack);

    vspace->free_stack(vspace, stack_top);
}

static inline void *
vspace_new_ipc_buffer(vspace_t *vspace, seL4_CPtr *page)
{
    assert(vspace);
    assert(page);
    assert(vspace->new_ipc_buffer);

    return vspace->new_ipc_buffer(vspace, page);
}

static inline void 
vspace_free_ipc_buffer(vspace_t *vspace, void *vaddr)
{
    assert(vspace);
    assert(vaddr);
    assert(vspace->free_ipc_buffer);

    vspace->free_ipc_buffer(vspace, vaddr);
}

static inline void *
vspace_new_pages(vspace_t *vspace, seL4_CapRights rights, size_t num_pages,
                size_t size_bits)
{
    assert(vspace);
    assert(vspace->new_pages);

    return vspace->new_pages(vspace, rights, num_pages, size_bits);
}

static inline int
vspace_new_pages_at_vaddr(vspace_t *vspace, void *vaddr, size_t num_pages, size_t size_bits,
        reservation_t *reservation)
{
    assert(vspace);
    assert(num_pages > 0);
    assert(reservation);
    assert(vspace->new_pages_at_vaddr);

    return vspace->new_pages_at_vaddr(vspace, vaddr, num_pages, size_bits, reservation);
}

static inline void
vspace_free_pages(vspace_t *vspace, void *vaddr, size_t num_pages, size_t size_bits)
{
    assert(vspace);
    assert(vaddr);
    assert(vspace->free_pages);

    vspace->free_pages(vspace, vaddr, num_pages, size_bits);
}

static inline void *
vspace_map_pages(vspace_t *vspace, seL4_CPtr caps[], seL4_CapRights rights,
                 size_t num_caps, size_t size_bits, int cacheable)
{
    assert(vspace);
    assert(num_caps > 0);
    assert(size_bits > 0);
    assert(vspace->map_pages);

    return vspace->map_pages(vspace, caps, rights, num_caps, size_bits, cacheable);
}

static inline int
vspace_map_pages_at_vaddr(vspace_t *vspace, seL4_CPtr caps[], void *vaddr, size_t num_pages,
        size_t size_bits, reservation_t *reservation)
{
    assert(vspace);
    assert(num_pages > 0);
    assert(size_bits > 0);
    assert(vaddr);
    assert(reservation);
    assert(vspace->map_pages_at_vaddr);

    return vspace->map_pages_at_vaddr(vspace, caps, vaddr, num_pages, size_bits, reservation);
}

static inline void
vspace_unmap_pages(vspace_t *vspace, void *vaddr, size_t num_pages, size_t size_bits)
{
    assert(vspace);
    assert(num_pages > 0);
    assert(size_bits > 0);
    assert(vaddr);
    assert(vspace->unmap_pages);

    vspace->unmap_pages(vspace, vaddr, num_pages, size_bits);
}

static inline int
vspace_unmap_reserved_pages(vspace_t *vspace, void *vaddr, size_t num_pages, size_t size_bits, reservation_t *reservation)
{
    assert(vspace);
    assert(num_pages > 0);
    assert(size_bits > 0);
    assert(vaddr);
    assert(vspace->unmap_reserved_pages);

    return vspace->unmap_reserved_pages(vspace, vaddr, num_pages, size_bits, reservation);
}

static inline reservation_t *
vspace_reserve_range(vspace_t *vspace, size_t bytes, seL4_CapRights rights,
        int cacheable, void **vaddr)
{
    assert(vspace);
    assert(bytes > 0);
    assert(vaddr != NULL);
    assert(vspace->reserve_range);

    return vspace->reserve_range(vspace, bytes, rights, cacheable, vaddr);
}

static inline reservation_t *
vspace_reserve_range_at(vspace_t *vspace, void *vaddr,
        size_t bytes, seL4_CapRights rights, int cacheable)
{
    assert(vspace);
    assert(bytes > 0);
    assert(vspace->reserve_range_at);

    return vspace->reserve_range_at(vspace, vaddr, bytes, rights, cacheable);
}

static inline void
vspace_free_reservation(vspace_t *vspace, reservation_t *reservation)
{
    assert(vspace);
    assert(reservation);
    assert(vspace->free_reservation);

    vspace->free_reservation(vspace, reservation);
}

static inline seL4_CPtr
vspace_get_cap(vspace_t *vspace, void *vaddr)
{
    assert(vspace);
    assert(vaddr);
    assert(vspace->get_cap);

    return vspace->get_cap(vspace, vaddr);
}

/* Helper functions */

static inline void
vspace_maybe_call_allocated_object(vspace_t *vspace, vka_object_t object)
{
    assert(vspace != NULL);
    if (vspace->allocated_object != NULL) {
        assert(vspace->allocated_object != NULL);
        vspace->allocated_object(vspace->allocated_object_cookie, object);
    }
}

static inline seL4_CPtr
vspace_get_root(vspace_t *vspace)
{
    assert(vspace);
    return vspace->page_directory;
}

#endif /* _INTERFACE_VSPACE_H_ */


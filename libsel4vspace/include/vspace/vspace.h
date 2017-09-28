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

#include <assert.h>
#include <stddef.h>
#include <vka/object.h>
#include <vspace/page.h>

typedef struct vspace vspace_t;

typedef struct reservation {
    void *res;
} reservation_t;

/**
 * Configuration for vspace_new_pages functions
 */
typedef struct vspace_new_pages_config {
    /* If `NULL` then the mapping will be created at any available virtual
       address.  If vaddr is not `NULL` than the mapping will be at
       that virtual address if successful. */
    void *vaddr;
    /* Number of pages to be created and mapped */
    size_t num_pages;
    /* Number of bits for each page */
    size_t size_bits;
    /* Whether frames used to create pages can be device untyped or regular untyped */
    bool can_use_dev;
} vspace_new_pages_config_t;

/**
 * Returns a default configuration based on supplied parameters that can be passed to vspace_new_pages_with_config
 * or vspace_new_pages_at_vaddr_with_config.
 * @param  num_pages   number of pages in reservation
 * @param  size_bits   size bits of each page
 * @param  config      config struct to save configuration into
 * @return             0 on success.
 */
static inline int default_vspace_new_pages_config(size_t num_pages, size_t size_bits, vspace_new_pages_config_t *config) {
    if (num_pages == 0) {
        ZF_LOGW("attempt to create 0 pages");
        return -1;
    }

    config->vaddr = NULL;
    config->num_pages = num_pages;
    config->size_bits = size_bits;
    config->can_use_dev = false;
    return 0;
}

/**
 * Set vaddr of the config
 * @param  vaddr  vaddr to set. See documentation on vspace_new_pages_config_t.
 * @param  config config struct to save configuration into
 * @return        0 on success.
 */
static inline int vspace_new_pages_config_set_vaddr(void *vaddr, vspace_new_pages_config_t *config) {
    config->vaddr = vaddr;
    return 0;
}

/**
 * Set whether can use device untyped
 * @param  can_use_dev  `true` if can use device untyped. See documentation on vspace_new_pages_config_t.
 * @param  config config struct to save configuration into
 * @return        0 on success.
 */
static inline int vspace_new_pages_config_use_device_ut(bool can_use_dev, vspace_new_pages_config_t *config) {
    config->can_use_dev = can_use_dev;
    return 0;
}

/* IMPLEMENTATION INDEPENDANT FUNCTIONS - implemented by calling the implementation specific
 * function pointers */

/**
 * Reserve a range to map memory into later, aligned to 4K.
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
reservation_t vspace_reserve_range(vspace_t *vspace, size_t bytes,
                                   seL4_CapRights_t rights, int cacheable, void **vaddr);

/**
 * Share memory from one vspace to another.
 *
 * Make duplicate mappings of the from vspace in a contiguous region in the
 * to vspace. Pages are expected to already be mapped in the from vspace, or an error
 * will be returned.
 *
 * @param from      vspace to share memory from
 * @param to        vspace to share memory to
 * @param start     address to start sharing at
 * @param num_pages number of pages to share
 * @param size_bits size of pages in bits
 * @param rights    rights to map pages into the to vspace with.
 * @param cacheable cacheable attribute to map pages into the vspace with
 *
 * @return address of shared region in to, NULL on failure.
 */
void *vspace_share_mem(vspace_t *from, vspace_t *to, void *start, int num_pages,
                       size_t size_bits, seL4_CapRights_t rights, int cacheable);

/**
 * Create a virtually contiguous area of mapped pages.
 * This could be for shared memory or just allocating some pages.
 * Depending on the config passed in, this will create a reservation or
 * use an existing reservation
 * @param  vspace the virtual memory allocator used.
 * @param  config configuration for this function. See vspace_new_pages_config_t.
 * @param  rights the rights to map the pages in with
 * @return        vaddr at the start of the contiguous region
 *         NULL on failure.
 */
void * vspace_new_pages_with_config(vspace_t *vspace, vspace_new_pages_config_t *config, seL4_CapRights_t rights);

/**
 * Create a stack. The determines stack size.
 *
 * @param vspace the virtual memory allocator used.
 * @param n_pages number of 4k pages to allocate for the stack.
 *                A 4k guard page will also be reserved in the address space
 *                to prevent code from running off the created stack.
 *
 * @return virtual address of the top of the created stack.
 *         NULL on failure.
 */
void *vspace_new_sized_stack(vspace_t *vspace, size_t n_pages);

static inline void *
vspace_new_stack(vspace_t *vspace)
{
    return vspace_new_sized_stack(vspace, BYTES_TO_4K_PAGES(CONFIG_SEL4UTILS_STACK_SIZE));
}

/**
 * Free a stack. This will only free virtual resources, not physical resources.
 *
 * @param vspace the virtual memory allocator used.
 * @param stack_top the top of the stack as returned by vspace_new_stack.
 * @param n_pages number of 4k pages that were allocated for stack.
 *
 */
void vspace_free_sized_stack(vspace_t *vspace, void *stack_top, size_t n_pages);

static inline void
vspace_free_stack(vspace_t *vspace, void *stack_top)
{
    vspace_free_sized_stack(vspace, stack_top, BYTES_TO_4K_PAGES(CONFIG_SEL4UTILS_STACK_SIZE));
}

/**
 * Create an IPC buffer.
 *
 * @param[in] vspace the virtual memory allocator used.
 * @param[out] page capability to that the IPC buffer was mapped in with
 *
 * @return vaddr of the mapped in IPC buffer
 *         NULL on failure.
 */
void *vspace_new_ipc_buffer(vspace_t *vspace, seL4_CPtr *page);

/**
 * Free an IPC buffer. This will only free virtual resources, not physical resources.
 *
 *
 * @param vspace the virtual memory allocator used.
 * @param addr address the IPC buffer was mapped in to.
 *
 */
void vspace_free_ipc_buffer(vspace_t *vspace, void *addr);

/* IMPLEMENTATION SPECIFIC FUNCTIONS - function pointers of the vspace used */

typedef void *(*vspace_new_pages_fn)(vspace_t *vspace, seL4_CapRights_t rights,
                                     size_t num_pages, size_t size_bits);

typedef void *(*vspace_map_pages_fn)(vspace_t *vspace,
                                     seL4_CPtr caps[], uintptr_t cookies[], seL4_CapRights_t rights,
                                     size_t num_pages, size_t size_bits, int cacheable);

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
 * @param can_use_dev whether the underlying allocator can allocate object from ram device_untyped
 *                    (Setting this to true is normally safe unless when creating IPC buffers.)
 * @return seL4_NoError on success, -1 otherwise.
 */
typedef int (*vspace_new_pages_at_vaddr_fn)(vspace_t *vspace, void *vaddr, size_t num_pages,
                                            size_t size_bits, reservation_t reservation, bool can_use_dev);

/**
 * Map in existing page capabilities, using contiguos virtual memory at the specified virtual address.
 *
 * This will FAIL if the virtual address is already mapped in.
 *`
 * @param vspace the virtual memory allocator used.
 * @param seL4_CPtr caps array of caps to map in
 * @param uintptr_t cookies array of allocation cookies. Populate this if you want the vspace to
 *                         be able to free the caps for you with a vka. NULL acceptable.
 * @param size_bits size, in bits, of an individual page -- all pages must be the same size.
 * @param num_pages the number of pages to map in (must correspond to the size of the array).
 * @param reservation reservation to the range the allocation will take place in.
 *
 * @return seL4_NoError on success. -1 on failure.
 */
typedef int (*vspace_map_pages_at_vaddr_fn)(vspace_t *vspace, seL4_CPtr caps[], uintptr_t cookies[],
                                            void *vaddr, size_t num_pages,
                                            size_t size_bits, reservation_t reservation);

/* potential values for vspace_unmap_pages */
#define VSPACE_FREE ((vka_t *) 0xffffffff)
#define VSPACE_PRESERVE ((vka_t *) 0)

/**
 * Unmap in existing page capabilities that use contiguos virtual memory.
 *
 * This function can also free the cslots and frames that back the virtual memory in the region.
 * This can be done by the internal vka that the vspace was created with, or the user can provide
 * a vka to free with. The vka must be the same vka that the frame object and cslot were allocated with.
 *
 * Reservations are preserved.
 *
 * @param vspace the virtual memory allocator used.
 * @param vaddr the start of the contiguous region.
 * @param size_bits size, in bits, of an individual page -- all pages must be the same size.
 * @param num_pages the number of pages to map in (must correspond to the size of the array).
 * @param free interface to free frame objects and cslots with, options:
 *             + VSPACE_FREE to free the frames/cslots with the vspace internal vka,
 *             + VSPACE_PRESERVE to not free the frames/cslots or
 *             + a pointer to a custom vka to free the frames/cslots with.
 *
 */
typedef void (*vspace_unmap_pages_fn)(vspace_t *vspace, void *vaddr, size_t num_pages,
                                      size_t size_bits, vka_t *free);

/**
 * Tear down a vspace, freeing any memory allocated by the vspace itself.
 *
 * Like vspace_unmap_pages this function can also free the frames and cslots backing
 * the vspace, if a vka is provided.
 *
 * When using this function to tear down all backing frames/cslots the user MUST make sure
 * that any frames/cslots not allocated by the vka being used to free have already been unmapped
 * from the vspace *or* that the cookies for these custom mappings are set to 0.
 * If this is not done the vspace will attempt to use the wrong vka to free
 * frames and cslots resulting in allocator corruption.
 *
 * To completely free a vspace the user should also free any objects/cslots that the vspace
 * called vspace_allocated_object_fn on, as the vspace has essentially delegated control
 * of these objects/cslots to the user.
 *
 * @param vspace the vspace to tear down.
 * @param free vka to use to free the cslots/frames, options:
 *             + VSPACE_FREE to use the internal vka,
 *             + VSPACE_PRESERVE to not free the frames/cslots,
 *             + a pointer to a custom vka to free the frames/cslots with.
 */
typedef void (*vspace_tear_down_fn)(vspace_t *vspace, vka_t *free);

/**
 * Reserve a range to map memory into later.
 * Regions will be aligned to 4K boundaries.
 *
 * @param vspace the virtual memory allocator to use.
 * @param bytes the size in bytes to map.
 * @param size_bits size to align the range to
 * @param rights the rights to map the pages in with in this reservation
 * @param cacheable 1 if the pages should be mapped with cacheable attributes. 0 for DMA.
 * @param vaddr the virtual address of the reserved range will be returned here.
 *
 * @return a reservation to use when mapping pages in the range.
 */
typedef reservation_t (*vspace_reserve_range_aligned_fn)(vspace_t *vspace, size_t bytes, size_t size_bits,
                                                         seL4_CapRights_t rights, int cacheable, void **vaddr);

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
typedef reservation_t (*vspace_reserve_range_at_fn)(vspace_t *vspace, void *vaddr,
                                                    size_t bytes, seL4_CapRights_t rights, int cacheable);

/**
 * Free a reservation.
 *
 * This will not touch any pages, but will unreserve any reserved addresses in the reservation.
 *
 * @param vspace the virtual memory allocator to use.
 * @param reservation the reservation to free.
 */
typedef void (*vspace_free_reservation_fn)(vspace_t *vspace, reservation_t reservation);

/**
 * Free a reservation by vaddr.
 *
 * This will not touch any pages, but will unreserve any reserved addresses in the reservation.
 *
 * @param vspace the virtual memory allocator to use.
 * @param vaddr a vaddr in the reservation (will free entire reservation).
 */
typedef void (*vspace_free_reservation_by_vaddr_fn)(vspace_t *vspace, void *vaddr);

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
 * Get the vka allocation cookie for an the frame mapped at a virtual address.
 *
 * @param vspace the virtual memory allocator to use.
 * @param vaddr the virtual address to get the cap for.
 *
 * @return the allocation cookie mapped to this virtual address, 0 otherwise.
 */
typedef uintptr_t (*vspace_get_cookie_fn)(vspace_t *vspace, void *vaddr);

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

/* @return the page directory for this vspace
 */
typedef seL4_CPtr (*vspace_get_root_fn)(vspace_t *vspace);

/**
 * Share memory from one vspace to another at a specific address. Pages are expected
 * to already be mapped in the from vspace, or an error will be returned.
 *
 * @param from        vspace to share memory from
 * @param to          vspace to share memory to
 * @param start       address to start sharing at
 * @param num_pages   number of pages to share
 * @param size_bits   size of pages in bits
 * @param vaddr       vaddr to start sharing memory at
 * @param reservation reservation for that vaddr.
 *
 * @return 0 on success
 */
typedef int (*vspace_share_mem_at_vaddr_fn)(vspace_t *from, vspace_t *to, void *start, int num_pages, size_t size_bits, void *vaddr, reservation_t res);

/* Portable virtual memory allocation interface */
struct vspace {
    void *data;

    vspace_new_pages_fn new_pages;
    vspace_map_pages_fn map_pages;

    vspace_new_pages_at_vaddr_fn new_pages_at_vaddr;

    vspace_map_pages_at_vaddr_fn map_pages_at_vaddr;

    vspace_unmap_pages_fn unmap_pages;
    vspace_tear_down_fn tear_down;

    vspace_reserve_range_aligned_fn reserve_range_aligned;
    vspace_reserve_range_at_fn reserve_range_at;
    vspace_free_reservation_fn free_reservation;
    vspace_free_reservation_by_vaddr_fn free_reservation_by_vaddr;

    vspace_get_cap_fn get_cap;
    vspace_get_root_fn get_root;
    vspace_get_cookie_fn get_cookie;

    vspace_share_mem_at_vaddr_fn share_mem_at_vaddr;

    vspace_allocated_object_fn allocated_object;
    void *allocated_object_cookie;
};

/* convenient wrappers */

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
static inline void *
vspace_new_pages(vspace_t *vspace, seL4_CapRights_t rights,
                 size_t num_pages, size_t size_bits)
{
    if (vspace == NULL) {
        ZF_LOGE("vspace is NULL.");
        return NULL;
    }
    if (vspace->new_pages == NULL) {
        ZF_LOGE("Supplied vspace doesn't implement new_pages().");
        return NULL;
    }
    if (num_pages == 0) {
        ZF_LOGE("Called with num_pages == 0. Intentional?");
        return NULL;
    }
    if (!sel4_valid_size_bits(size_bits)) {
        ZF_LOGE("Invalid size_bits %zu", size_bits);
        return NULL;
    }

    return vspace->new_pages(vspace, rights, num_pages, size_bits);
}

/**
 * Map in existing page capabilities, using contiguos virtual memory.
 *
 * @param vspace the virtual memory allocator used.
 * @param seL4_CPtr caps array of caps to map in
 * @param uint32_t cookies array of allocation cookies. Populate this if you want the vspace to
 *                         be able to free the caps for you with a vka. NULL acceptable.
 * @param rights the rights to map the pages in with
 * @param size_bits size, in bits, of an individual page -- all pages must be the same size.
 * @param num_pages the number of pages to map in (must correspond to the size of the array).
 * @param cacheable 1 if the pages should be mapped with cacheable attributes. 0 for DMA.
 *
 * @return vaddr at the start of the device mapping
 *         NULL on failure.
 */
static inline void *
vspace_map_pages(vspace_t *vspace, seL4_CPtr caps[], uintptr_t cookies[],
                 seL4_CapRights_t rights, size_t num_pages, size_t size_bits,
                 int cacheable)
{
    if (vspace == NULL) {
        ZF_LOGE("vspace is NULL.");
        return NULL;
    }
    if (vspace->new_pages == NULL) {
        ZF_LOGE("Supplied vspace doesn't implement map_pages().");
        return NULL;
    }
    if (num_pages == 0) {
        ZF_LOGE("Called with num_pages == 0. Intentional?");
        return NULL;
    }
    if (!sel4_valid_size_bits(size_bits)) {
        ZF_LOGE("Invalid size_bits %zu", size_bits);
        return NULL;
    }

    return vspace->map_pages(vspace, caps, cookies, rights,
                             num_pages, size_bits, cacheable);
}

static inline int
vspace_new_pages_at_vaddr_with_config(vspace_t *vspace, vspace_new_pages_config_t *config, reservation_t res) {
    if (vspace == NULL) {
        ZF_LOGE("vspace is NULL");
        return -1;
    }
    if (res.res == NULL) {
        ZF_LOGE("reservation is required");
    }
    return vspace->new_pages_at_vaddr(vspace, config->vaddr, config->num_pages, config->size_bits, res, config->can_use_dev);
}

static inline int
vspace_new_pages_at_vaddr(vspace_t *vspace, void *vaddr, size_t num_pages, size_t size_bits,
                          reservation_t reservation)
{
    if (vspace == NULL) {
        ZF_LOGE("vspace is NULL");
        return -1;
    }

    if (vspace->new_pages_at_vaddr == NULL) {
        ZF_LOGE("Unimplemented");
        return -1;
    }
    vspace_new_pages_config_t config;
    if (default_vspace_new_pages_config(num_pages, size_bits, &config)) {
        ZF_LOGE("Failed to create config");
        return -1;
    }
    if (vspace_new_pages_config_set_vaddr(vaddr, &config)) {
        ZF_LOGE("Failed to set vaddr");
        return -1;
    }
    return vspace_new_pages_at_vaddr_with_config(vspace, &config, reservation);
}

static inline int
vspace_map_pages_at_vaddr(vspace_t *vspace, seL4_CPtr caps[], uintptr_t cookies[], void *vaddr,
                          size_t num_pages, size_t size_bits, reservation_t reservation)
{
    if (vspace == NULL) {
        ZF_LOGE("vspace is NULL");
        return -1;
    }

    if (num_pages == 0) {
        ZF_LOGW("Attempt to map 0 pages");
        return -1;
    }

    if (vaddr == NULL) {
        ZF_LOGW("Mapping NULL");
    }

    if (vspace->map_pages_at_vaddr == NULL) {
        ZF_LOGW("Unimplemented\n");
        return -1;
    }

    return vspace->map_pages_at_vaddr(vspace, caps, cookies, vaddr, num_pages, size_bits, reservation);
}

static inline void
vspace_unmap_pages(vspace_t *vspace, void *vaddr, size_t num_pages, size_t size_bits, vka_t *vka)
{

    if (vspace == NULL) {
        ZF_LOGE("vspace is NULL");
        return;
    }

    if (num_pages == 0) {
        printf("Num pages : %zu\n", num_pages);
        ZF_LOGW("Attempt to unmap 0 pages");
        return;
    }

    if (vaddr == NULL) {
        ZF_LOGW("Attempt to unmap NULL\n");
    }

    if (vspace->unmap_pages == NULL) {
        ZF_LOGE("Not implemented\n");
        return;
    }

    vspace->unmap_pages(vspace, vaddr, num_pages, size_bits, vka);
}

static inline void
vspace_tear_down(vspace_t *vspace, vka_t *vka)
{
    if (vspace == NULL) {
        ZF_LOGE("vspace is NULL");
        return;
    }

    if (vspace->tear_down == NULL) {
        ZF_LOGE("Not implemented");
        return;
    }
    vspace->tear_down(vspace, vka);
}

static inline reservation_t
vspace_reserve_range_aligned(vspace_t *vspace, size_t bytes, size_t size_bits,
                             seL4_CapRights_t rights, int cacheable, void **vaddr)
{
    reservation_t error = { .res = 0 };

    if (vspace == NULL) {
        ZF_LOGE("vspace is NULL");
        return error;
    }

    if (vspace->reserve_range_aligned == NULL) {
        ZF_LOGE("Not implemented");
        return error;
    }

    if (bytes == 0) {
        ZF_LOGE("Attempt to reserve 0 length range");
        return error;
    }

    if (vaddr == NULL) {
        ZF_LOGE("Cannot store result at NULL");
        return error;
    }

    return vspace->reserve_range_aligned(vspace, bytes, size_bits, rights, cacheable, vaddr);
}

static inline reservation_t
vspace_reserve_range_at(vspace_t *vspace, void *vaddr,
                        size_t bytes, seL4_CapRights_t rights, int cacheable)
{
    reservation_t error = { .res = 0 };

    if (vspace == NULL) {
        ZF_LOGE("vspace is NULL");
        return error;
    }

    if (vspace->reserve_range_at == NULL) {
        ZF_LOGE("Not implemented");
        return error;
    }

    if (bytes == 0) {
        ZF_LOGE("Attempt to reserve 0 length range");
        return error;
    }

    return vspace->reserve_range_at(vspace, vaddr, bytes, rights, cacheable);
}

static inline void
vspace_free_reservation(vspace_t *vspace, reservation_t reservation)
{
    if (vspace == NULL) {
        ZF_LOGE("vspace is NULL");
        return;
    }

    if (vspace->free_reservation == NULL) {
        ZF_LOGE("Not implemented");
        return;
    }

    vspace->free_reservation(vspace, reservation);
}

static inline void
vspace_free_reservation_by_vaddr(vspace_t *vspace, void *vaddr)
{
    if (vspace == NULL) {
        ZF_LOGE("vspace is NULL");
        return;
    }

    if (vspace->free_reservation_by_vaddr == NULL) {
        ZF_LOGE("Not implemented");
        return;
    }

    vspace->free_reservation_by_vaddr(vspace, vaddr);
}

static inline seL4_CPtr
vspace_get_cap(vspace_t *vspace, void *vaddr)
{

    if (vspace == NULL) {
        ZF_LOGE("vspace is NULL");
        return seL4_CapNull;
    }

    if (vaddr == NULL) {
        ZF_LOGW("Warning: null address");
    }

    if (vspace->get_cap == NULL) {
        ZF_LOGE("Not implemented\n");
        return seL4_CapNull;
    }

    return vspace->get_cap(vspace, vaddr);
}

static inline uintptr_t
vspace_get_cookie(vspace_t *vspace, void *vaddr)
{
    if (vspace == NULL) {
        ZF_LOGE("vspace is NULL");
        return 0;
    }

    if (vaddr == NULL) {
        /* only warn as someone might do this intentionally? */
        ZF_LOGW("Warning: null address");
    }

    if (vspace->get_cookie == NULL) {
        ZF_LOGE("Not implemented");
        return 0;
    }

    return vspace->get_cookie(vspace, vaddr);
}

/* Helper functions */

static inline void
vspace_maybe_call_allocated_object(vspace_t *vspace, vka_object_t object)
{
    if (vspace == NULL) {
        ZF_LOGF("vspace is NULL");
    }

    if (vspace->allocated_object != NULL) {
        vspace->allocated_object(vspace->allocated_object_cookie, object);
    }
}

static inline seL4_CPtr
vspace_get_root(vspace_t *vspace)
{
    if (vspace == NULL) {
        ZF_LOGE("vspace is NULL");
        return seL4_CapNull;
    }
    if (vspace->get_root == NULL) {
        ZF_LOGE("Not implemented");
        return seL4_CapNull;
    }
    return vspace->get_root(vspace);
}

static inline int
vspace_share_mem_at_vaddr(vspace_t *from, vspace_t *to, void *start, int num_pages,
                          size_t size_bits, void *vaddr, reservation_t res)
{

    if (num_pages == 0) {
        /* nothing to do */
        return -1;
    } else if (num_pages < 0) {
        ZF_LOGE("Attempted to share %d pages\n", num_pages);
        return -1;
    }

    if (from == NULL) {
        ZF_LOGE("From vspace does not exist");
        return -1;
    }

    if (to == NULL) {
        ZF_LOGE("To vspace does not exist");
        return -1;
    }

    if (start == NULL || vaddr == NULL) {
        ZF_LOGE("Cannot share memory at NULL");
        return -1;
    }

    if (from->share_mem_at_vaddr == NULL) {
        ZF_LOGE("Not implemented for this vspace\n");
        return -1;
    }

    return from->share_mem_at_vaddr(from, to, start, num_pages, size_bits, vaddr, res);
}


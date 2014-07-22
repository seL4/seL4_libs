/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/**
 * @file allocman.h
 *
 * @brief The allocman is system for resolving dependencies between allocators for different resources
 *
 * Allocations need to go via the allocation manager in order to ensure
 * the correct watermark levels of resources are maintained. While an
 * individual manager is free to directly give away resources, if it
 * calls into the allocation manager then that manager may be recursively
 * invoked. Performing allocations from the allocation manager is also
 * the only way to allocate the final watermark resources when memory
 * becomes exhausted.
 *
 * It is generally desirable that a free operation does not have any
 * allocation calls in it. If an allocator does wish to allocate a
 * resource when performing a free it must accept that its allocation
 * function could be called as a result. In a similar manner if your
 * allocation function frees resources your free function may be recursively
 * called.
 *
 * There are (generally) two different types of allocators. Those that are
 * linked to an allocation manager, and those that are not. Typically the
 * only sort of manager you would not want linked to an allocation manager
 * is a cspace manager (if you are managing a clients cspace). Although you
 * could also create an untyped manager if you do not want to give clients
 * untypeds directly, but still want to have a fixed untyped pool reserved
 * for it.
 *
 * Possibility exists for much foot shooting with any allocators. A typical
 * desire might be to create a sub allocator (such as a cspace manager),
 * use an already existing allocation manager to back all of its allocations,
 * and then destroy that cspace manager at some point to release all its resources.
 * There are no guarantees that this will work. If all requests to the sub allocator
 * use the same allocation manager to perform book keeping requests, and the
 * sub allocator is told to free using that same allocation manager then all
 * should work. But this is strictly up to using your allocators correctly,
 * and knowing how they work.
 */

#ifndef _ALLOCMAN_H_
#define _ALLOCMAN_H_

#include <assert.h>
#include <autoconf.h>
#include <sel4/types.h>
#include <allocman/util.h>
#include <allocman/cspace/cspace.h>
#include <allocman/mspace/mspace.h>
#include <allocman/utspace/utspace.h>
#include <vka/cspacepath_t.h>

/**
 * Describes a reservation chunk for the memory system.
 * Used by {@link #allocman_configure_mspace_reserve}
 */
struct allocman_mspace_chunk {
    uint32_t size;
    uint32_t count;
};

/**
 * Describes a reservation chunk for the untyped system.
 * Used by {@link #allocman_configure_utspace_reserve}
 */
struct allocman_utspace_chunk {
    uint32_t size_bits;
    seL4_Word type;
    uint32_t count;
};

/**
 * Internal data structure for describing an untyped allocation in
 * the reservation system
 */
struct allocman_utspace_allocation {
    uint32_t cookie;
    cspacepath_t slot;
};

struct allocman_freed_mspace_chunk {
    void *ptr;
    uint32_t size;
};

struct allocman_freed_utspace_chunk {
    uint32_t size_bits;
    uint32_t cookie;
};

/**
 * The allocman itself. This is generally the only type you will need to pass around
 * to deal with allocation. It is declared in full here so that the compiler is able
 * to calculate its size so it can be allocated on stacks/globals etc as required
 */
typedef struct allocman {
    /* link to our underlying allocators. some are lazily added. the mspace will always be here,
     * and have_mspace can be used to check if the allocman is initialized at all */
    int have_mspace;
    struct mspace_interface mspace;
    int have_cspace;
    struct cspace_interface cspace;
    int have_utspace;
    struct utspace_interface utspace;

    /* Flag that tracks whether any alloc/free/other function has been entered yet */
    int in_operation;

    /* Counts that track re-entry into each specific alloc/free function */
    uint32_t cspace_alloc_depth;
    uint32_t cspace_free_depth;
    uint32_t utspace_alloc_depth;
    uint32_t utspace_free_depth;
    uint32_t mspace_alloc_depth;
    uint32_t mspace_free_depth;

    /* Track whether the watermark is currently refilled so we don't recursively do it */
    int refilling_watermark;
    /* Has a watermark resource been used. This is just an optimization */
    int used_watermark;

    /* track resources that we have not yet been able to free due to circular dependencies */
    uint32_t desired_freed_slots;
    uint32_t num_freed_slots;
    cspacepath_t *freed_slots;

    uint32_t desired_freed_mspace_chunks;
    uint32_t num_freed_mspace_chunks;
    struct allocman_freed_mspace_chunk *freed_mspace_chunks;

    uint32_t desired_freed_utspace_chunks;
    uint32_t num_freed_utspace_chunks;
    struct allocman_freed_utspace_chunk *freed_utspace_chunks;

    /* cspace watermark resources */
    uint32_t desired_cspace_slots;
    uint32_t num_cspace_slots;
    cspacepath_t *cspace_slots;

    /* mspace watermark resources */
    uint32_t num_mspace_chunks;
    struct allocman_mspace_chunk *mspace_chunk;
    uint32_t *mspace_chunk_count;
    void ***mspace_chunks;

    /* utspace watermark resources */
    uint32_t num_utspace_chunks;
    struct allocman_utspace_chunk *utspace_chunk;
    uint32_t *utspace_chunk_count;
    struct allocman_utspace_allocation **utspace_chunks;
} allocman_t;

/**
 * Allocates 'real' memory from the allocator
 *
 * @param alloc Allocman to allocate from
 * @param bytes Size in bytes to allocate
 * @param _error (Optional) set to 0 on success
 *
 * @return returns pointer to allocated memory
 */
void *allocman_mspace_alloc(allocman_t *alloc, uint32_t bytes, int *_error);

/**
 * Frees 'real' memory, as previously allocated by {@link #allocman_mspace_alloc}
 *
 * @param alloc Allocman to allocate from
 * @param ptr Allocated memory (as returned by {@link #allocman_mspace_alloc}
 * @param bytes Size in bytes of the allocation to free. Allocations cannot be partially freed
 */
void allocman_mspace_free(allocman_t *alloc, void *ptr, uint32_t bytes);

/**
 * Allocates a cslot from the allocator
 *
 * @param alloc Allocman to allocate from
 * @param slot Stores details of the allocated slot
 *
 * @return returns 0 on sucess
 */
int allocman_cspace_alloc(allocman_t *alloc, cspacepath_t *slot);

/**
 * Frees a cslot from the allocator, as previously allocated by {@link #allocman_cspace_alloc}.
 * To avoid the need to keep cspacepath_t's laying around, it is guaruanteed that
 * (*slot) == allocman_cspace_make_path(alloc, slot->capPtr). So if needed you can simply store
 * the capPtr and reconstruct the path before calling free.
 *
 * @param alloc Allocman to allocate from
 * @param slot The slot to free.
 *
 * @return returns 0 on sucess
 */
void allocman_cspace_free(allocman_t *alloc, cspacepath_t *slot);

/**
 * Converts a seL4_CPtr into a cspacepath_t using the cspace attached to the allocman.
 * If the slot is not valid in that cspace then the return path is completely undefined.
 *
 * @param alloc Allocman to allocate from
 * @param slot The slot to convert
 *
 * @return cspacepath_t of the given slot
 */
static inline cspacepath_t allocman_cspace_make_path(allocman_t *alloc, seL4_CPtr slot) {
    assert(alloc->have_cspace);
    return alloc->cspace.make_path(alloc->cspace.cspace, slot);
}

/**
 * Allocates a portion of untyped memory, and retypes it into the desired object for you.
 *
 * @param alloc Allocman to allocate from
 * @param size_bits The size in bits of the memory that will be required to store this object.
    This is different to seL4_Untyped_Retype for allocating seL4_CapTableObjects
 * @param type The seL4 type of the object being allocated
 * @param path A path to a location to put the allocated object (this must be a valid empty slot)
 * @param _error (Optional) set to 0 on success
 *
 * @return Returns a cookie that can be used in future to free this allocation
 */
uint32_t allocman_utspace_alloc(allocman_t *alloc, uint32_t size_bits, seL4_Word type, cspacepath_t *path, int *_error);

/**
 * Returns a portion of untyped memory back to the allocator. It is assumed that this
 * memory is now unused, and every capability to this memory has been deleted (including
 * the one created by {@link allocman_utspace_alloc}
 *
 * @param alloc Allocman to allocate from
 * @param size_bits The size in bits of the memory that was required to store this object.
    This is different to seL4_Untyped_Retype for seL4_CapTableObjects
 * @param cookie The cookie representing this allocation (as returned by {@link allocman_utspace_alloc}
 */
void allocman_utspace_free(allocman_t *alloc, uint32_t cookie, uint32_t size_bits);

/**
 * Initialize a new allocman. all it requires is a memory allocator, everything will be boot strapped from it
 *
 * @param alloc Allocman structure to initialize
 * @param mspace Memory allocator. This will be permanently linked to this allocator and must keep existing
 *
 * @return returns 0 on success
 */
int allocman_create(allocman_t *alloc, struct mspace_interface mspace);

/**
 * Attempts to fill the reserves of the allocator. This can be used if the underlying allocators have been modified,
 * for instance by having resources added, or as a way to query the health of the allocman
 *
 * @param alloc The allocman to fill reserves of
 *
 * @return returns 0 if reserves are full
 */
int allocman_fill_reserves(allocman_t *alloc);

/**
 * Attach an untyped allocator to an allocman.
 *
 * @param alloc The allocman to attach to
 * @param utspace untyped allocator to attach. This wil lbe permanently linked to this allocator and must keep existing
 *
 * @return returns 0 on success
 */
int allocman_attach_utspace(allocman_t *alloc, struct utspace_interface utspace);

/**
 * Attach a cspace manager to an allocman.
 *
 * @param alloc The allocman to attach to
 * @param cspace The cspace manager to attach. This wil lbe permanently linked to this allocator and must keep existing
 *
 * @return returns 0 on success
 */
int allocman_attach_cspace(allocman_t *alloc, struct cspace_interface cspace);

/**
 * Configure the memory reserve for the allocator
 *
 * @param alloc The allocman to configure
 * @param chunk Description of the memory reserve
 *
 * @return returns 0 on success
 */
int allocman_configure_mspace_reserve(allocman_t *alloc, struct allocman_mspace_chunk chunk);

/**
 * Configure the untyped reserve for the allocator
 *
 * @param alloc The allocman to configure
 * @param chunk Description of the untyped reserve
 *
 * @return returns 0 on success
 */
int allocman_configure_utspace_reserve(allocman_t *alloc, struct allocman_utspace_chunk chunk);

/**
 * Configure the cspace reserve for the allocator
 *
 * @param alloc The allocman to configure
 * @param num Number of cslots to hold in reserve
 *
 * @return returns 0 on success
 */
int allocman_configure_cspace_reserve(allocman_t *alloc, uint32_t num);

/**
 * Configure the maximul number of freed cptrs we can store. This is required for
 * scenarios where an allocator cannot handle a recursive call, but we would like to not
 * leak memory
 *
 * @param alloc The allocman to configure
 * @param num Maximum number of slots to handle
 *
 * @return returns 0 on success
 */
int allocman_configure_max_freed_slots(allocman_t *alloc, uint32_t num);

/**
 * Configure the maximul number of freed memory objects we can store. This is required for
 * scenarios where an allocator cannot handle a recursive call, but we would like to not
 * leak memory
 *
 * @param alloc The allocman to configure
 * @param num Maxmimum number of chunks to handle
 *
 * @return returns 0 on success
 */
int allocman_configure_max_freed_memory_chunks(allocman_t *alloc, uint32_t num);

/**
 * Configure the maximul number of freed untyped objects we can store. This is required for
 * scenarios where an allocator cannot handle a recursive call, but we would like to not
 * leak memory
 *
 * @param alloc The allocman to configure
 * @param num Maxmimum number of chunks to handle
 *
 * @return returns 0 on success
 */
int allocman_configure_max_freed_untyped_chunks(allocman_t *alloc, uint32_t num);

/**
 * Add additional untyped objects to the underlying untyped manager. This allows additional
 * resources to be injected after the allocman has started
 *
 * @param alloc The allocman to add to
 * @param num Number of untypeds to add
 * @param uts Path to each of the untyped to add. untyped is assumed to be at depth 32 from this threads cspace_root
 * @param size_bits Size, in bits, of each of the untypeds
 * @param paddr Optional parameter specifying the physical address of each of the untypeds
 *
 * @return returns 0 on success
 */
static inline int allocman_utspace_add_uts(allocman_t *alloc, uint32_t num, cspacepath_t *uts, uint32_t *size_bits, uint32_t *paddr) {
    int error;
    assert(alloc->have_utspace);
    error = alloc->utspace.add_uts(alloc, alloc->utspace.utspace, num, uts, size_bits, paddr);
    if (error) {
        return error;
    }
    allocman_fill_reserves(alloc);
    return 0;
}

/**
 * Retrieves the physical address for an allocated untyped object
 *
 * @param alloc The allocman to query
 * @param cookie Cookie to the allocated untyped object
 * @param size_bits Size of the allocated untyped object
 *
 * @return Physical address of the object
 */
static inline uint32_t allocman_utspace_paddr(allocman_t *alloc, uint32_t cookie, uint32_t size_bits) {
    assert(alloc->have_utspace);
    return alloc->utspace.paddr(alloc->utspace.utspace, cookie, size_bits);
}

#endif

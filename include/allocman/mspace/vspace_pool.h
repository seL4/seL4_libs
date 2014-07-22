/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _ALLOCMAN_MSPACE_VSPACE_POOL_H_
#define _ALLOCMAN_MSPACE_VSPACE_POOL_H_

#include <autoconf.h>
#include <stdint.h>
#include <sel4/types.h>
#include <allocman/mspace/mspace.h>
#include <allocman/mspace/k_r_malloc.h>
#include <vspace/vspace.h>

/* Performs allocation from a virtual pool of memory. It takes both a vspace
 * manager as well as a virtual range because we need to construct sbrk semantics
 * which requires a contiguous virtual address range. But we will use the vspace
 * manager to ensure we do not accidentally clobber somebody else. This memory manager
 * must *not* be used as a backing memory manager for somethign that is given to
 * the vspace library as the resulting circular dependency is unresolvable
 */

struct mspace_vspace_pool_config {
    uint32_t vstart;
    reservation_t *reservation;
    vspace_t vspace;
};

typedef struct mspace_vspace_pool {
    uint32_t pool_ptr;
    uint32_t pool_top;
    /* reservation inside the vspace_t */
    reservation_t *reservation;
    vspace_t vspace;
    mspace_k_r_malloc_t k_r_malloc;
    struct allocman *morecore_alloc;
} mspace_vspace_pool_t;

void mspace_vspace_pool_create(mspace_vspace_pool_t *vspace_pool, struct mspace_vspace_pool_config config);

void *_mspace_vspace_pool_alloc(struct allocman *alloc, void *_vspace_pool, uint32_t bytes, int *error);
void _mspace_vspace_pool_free(struct allocman *alloc, void *_vspace_pool, void *ptr, uint32_t bytes);

static inline struct mspace_interface mspace_vspace_pool_make_interface(mspace_vspace_pool_t *vspace_pool) {
    return (struct mspace_interface){
        .alloc = _mspace_vspace_pool_alloc,
        .free = _mspace_vspace_pool_free,
        .properties = ALLOCMAN_DEFAULT_PROPERTIES,
        .mspace = vspace_pool
    };
}

#endif

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _ALLOCMAN_MSPACE_FIXED_POOL_H_
#define _ALLOCMAN_MSPACE_FIXED_POOL_H_

#include <autoconf.h>
#include <stdint.h>
#include <sel4/types.h>
#include <allocman/mspace/mspace.h>
#include <allocman/mspace/k_r_malloc.h>

/* Performs allocation from a fixed pool of memory */

struct mspace_fixed_pool_config {
    void *pool;
    uint32_t size;
};

typedef struct mspace_fixed_pool {
    uint32_t pool_ptr;
    uint32_t remaining;
    mspace_k_r_malloc_t k_r_malloc;
} mspace_fixed_pool_t;

void mspace_fixed_pool_create(mspace_fixed_pool_t *fixed_pool, struct mspace_fixed_pool_config config);

void *_mspace_fixed_pool_alloc(struct allocman *alloc, void *_fixed_pool, uint32_t bytes, int *error);
void _mspace_fixed_pool_free(struct allocman *alloc, void *_fixed_pool, void *ptr, uint32_t bytes);

static inline struct mspace_interface mspace_fixed_pool_make_interface(mspace_fixed_pool_t *fixed_pool) {
    return (struct mspace_interface){
        .alloc = _mspace_fixed_pool_alloc,
        .free = _mspace_fixed_pool_free,
        .properties = ALLOCMAN_DEFAULT_PROPERTIES,
        .mspace = fixed_pool
    };
}

#endif

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
#include <stdlib.h>
#include <sel4/types.h>
#include <allocman/mspace/mspace.h>
#include <allocman/mspace/k_r_malloc.h>

/* Performs allocation from a pool of virtual memory */

struct mspace_virtual_pool_config {
    void *vstart;
    size_t size;
    seL4_CPtr pd;
};

typedef struct mspace_virtual_pool {
    void *pool_ptr;
    void *pool_top;
    void *pool_limit;
    seL4_CPtr pd;
    mspace_k_r_malloc_t k_r_malloc;
    struct allocman *morecore_alloc;
} mspace_virtual_pool_t;

void mspace_virtual_pool_create(mspace_virtual_pool_t *virtual_pool, struct mspace_virtual_pool_config config);

void *_mspace_virtual_pool_alloc(struct allocman *alloc, void *_virtual_pool, size_t bytes, int *error);
void _mspace_virtual_pool_free(struct allocman *alloc, void *_virtual_pool, void *ptr, size_t bytes);

static inline struct mspace_interface mspace_virtual_pool_make_interface(mspace_virtual_pool_t *virtual_pool) {
    return (struct mspace_interface){
        .alloc = _mspace_virtual_pool_alloc,
        .free = _mspace_virtual_pool_free,
        .properties = ALLOCMAN_DEFAULT_PROPERTIES,
        .mspace = virtual_pool
    };
}


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
#include <allocman/mspace/fixed_pool.h>
#include <allocman/mspace/virtual_pool.h>

/* This memory allocator supports have a fixed pool of memory, as well as a
 * virtual pool that can be attached while running. */

typedef struct mspace_dual_pool {
    size_t fixed_pool_start;
    size_t fixed_pool_end;
    mspace_fixed_pool_t fixed_pool;
    int have_virtual_pool;
    mspace_virtual_pool_t virtual_pool;
} mspace_dual_pool_t;

void mspace_dual_pool_create(mspace_dual_pool_t *dual_pool, struct mspace_fixed_pool_config config);
void mspace_dual_pool_attach_virtual(mspace_dual_pool_t *dual_pool, struct mspace_virtual_pool_config config);

void *_mspace_dual_pool_alloc(struct allocman *alloc, void *_dual_pool, size_t bytes, int *error);
void _mspace_dual_pool_free(struct allocman *alloc, void *_dual_pool, void *ptr, size_t bytes);

static inline struct mspace_interface mspace_dual_pool_make_interface(mspace_dual_pool_t *dual_pool) {
    return (struct mspace_interface){
        .alloc = _mspace_dual_pool_alloc,
        .free = _mspace_dual_pool_free,
        .properties = ALLOCMAN_DEFAULT_PROPERTIES,
        .mspace = dual_pool
    };
}


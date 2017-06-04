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

#include <allocman/mspace/dual_pool.h>
#include <allocman/allocman.h>
#include <allocman/util.h>
#include <stdlib.h>

void mspace_dual_pool_create(mspace_dual_pool_t *dual_pool, struct mspace_fixed_pool_config config)
{
    *dual_pool = (mspace_dual_pool_t) {
        .fixed_pool_start = (uintptr_t)config.pool,
        .fixed_pool_end = (size_t)config.pool + config.size,
        .have_virtual_pool = 0
    };
    mspace_fixed_pool_create(&dual_pool->fixed_pool, config);
}

void mspace_dual_pool_attach_virtual(mspace_dual_pool_t *dual_pool, struct mspace_virtual_pool_config config) {
    mspace_virtual_pool_create(&dual_pool->virtual_pool, config);
    dual_pool->have_virtual_pool = 1;
}

void *_mspace_dual_pool_alloc(allocman_t *alloc, void *_dual_pool, size_t bytes, int *error)
{
    mspace_dual_pool_t *dual_pool = (mspace_dual_pool_t*)_dual_pool;
    /* if we have a virtual pool try and use this instead of wasting time in the fixed pool */
    if (dual_pool->have_virtual_pool) {
        int _error;
        void *ret = _mspace_virtual_pool_alloc(alloc, &dual_pool->virtual_pool, bytes, &_error);
        if (!_error) {
            SET_ERROR(error, 0);
            return ret;
        }
    }
    /* try from the fixed pool */
    return _mspace_fixed_pool_alloc(alloc, &dual_pool->fixed_pool, bytes, error);
}

void _mspace_dual_pool_free(struct allocman *alloc, void *_dual_pool, void *ptr, size_t bytes)
{
    mspace_dual_pool_t *dual_pool = (mspace_dual_pool_t*)_dual_pool;
    if ( (uintptr_t)ptr >= dual_pool->fixed_pool_start && (uintptr_t)ptr < dual_pool->fixed_pool_end) {
        _mspace_fixed_pool_free(alloc, &dual_pool->fixed_pool, ptr, bytes);
    } else {
        _mspace_virtual_pool_free(alloc, &dual_pool->virtual_pool, ptr, bytes);
    }
}

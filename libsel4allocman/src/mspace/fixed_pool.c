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

#include <allocman/mspace/fixed_pool.h>
#include <allocman/allocman.h>
#include <allocman/util.h>
#include <stdlib.h>

static k_r_malloc_header_t *_morecore(size_t cookie, mspace_k_r_malloc_t *k_r_malloc, size_t new_units)
{
    size_t new_size;
    k_r_malloc_header_t *new_header;
    mspace_fixed_pool_t *fixed_pool = (mspace_fixed_pool_t*)cookie;
    new_size = new_units * sizeof(k_r_malloc_header_t);
    if (new_size > fixed_pool->remaining) {
        return NULL;
    }
    new_header = (k_r_malloc_header_t*)fixed_pool->pool_ptr;
    fixed_pool->pool_ptr += new_size;
    fixed_pool->remaining -= new_size;
    return new_header;
}

void mspace_fixed_pool_create(mspace_fixed_pool_t *fixed_pool, struct mspace_fixed_pool_config config)
{
    size_t padding;
    fixed_pool->pool_ptr = (uintptr_t)config.pool;
    fixed_pool->remaining = config.size;
    padding = fixed_pool->pool_ptr % sizeof(k_r_malloc_header_t);
    fixed_pool->pool_ptr += padding;
    fixed_pool->remaining -= padding;
    mspace_k_r_malloc_init(&fixed_pool->k_r_malloc, (size_t)fixed_pool, _morecore);
}

void *_mspace_fixed_pool_alloc(struct allocman *alloc, void *_fixed_pool, size_t bytes, int *error)
{
    void *ret;
    mspace_fixed_pool_t *fixed_pool = (mspace_fixed_pool_t*)_fixed_pool;
    ret = mspace_k_r_malloc_alloc(&fixed_pool->k_r_malloc, bytes);
    if (ret == NULL) {
        SET_ERROR(error, 1);
    } else {
        SET_ERROR(error, 0);
    }
    return ret;
}

void _mspace_fixed_pool_free(struct allocman *alloc, void *_fixed_pool, void *ptr, size_t bytes)
{
    mspace_fixed_pool_t *fixed_pool = (mspace_fixed_pool_t*)_fixed_pool;
    mspace_k_r_malloc_free(&fixed_pool->k_r_malloc, ptr);
}

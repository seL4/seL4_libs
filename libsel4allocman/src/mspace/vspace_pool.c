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

#include <allocman/mspace/vspace_pool.h>
#include <allocman/allocman.h>
#include <allocman/util.h>
#include <stdlib.h>
#include <sel4/sel4.h>

/* we will arbitrarily use 4k pages. Be nice if this was configureable for
 * modern systems where 4k is small and wasteful
 */
#define PAGE_SIZE_BITS 12

static k_r_malloc_header_t *_morecore(size_t cookie, mspace_k_r_malloc_t *k_r_malloc, size_t new_units)
{
    size_t new_size;
    k_r_malloc_header_t *new_header;
    mspace_vspace_pool_t *vspace_pool = (mspace_vspace_pool_t*)cookie;
    new_size = new_units * sizeof(k_r_malloc_header_t);
    while (vspace_pool->pool_ptr + new_size > vspace_pool->pool_top) {
        int error;
        error = vspace_new_pages_at_vaddr(&vspace_pool->vspace, (void*)vspace_pool->pool_top, 1, PAGE_SIZE_BITS, vspace_pool->reservation);
        if (error != seL4_NoError) {
            return NULL;
        }
        vspace_pool->pool_top += BIT(PAGE_SIZE_BITS);
    }
    new_header = (k_r_malloc_header_t*)vspace_pool->pool_ptr;
    vspace_pool->pool_ptr += new_size;
    return new_header;
}

void mspace_vspace_pool_create(mspace_vspace_pool_t *vspace_pool, struct mspace_vspace_pool_config config)
{
    vspace_pool->pool_ptr = config.vstart;
    vspace_pool->pool_top = config.vstart;
    vspace_pool->reservation = config.reservation;
    vspace_pool->vspace = config.vspace;
    vspace_pool->morecore_alloc = NULL;

    mspace_k_r_malloc_init(&vspace_pool->k_r_malloc, (size_t)vspace_pool, _morecore);
}

void *_mspace_vspace_pool_alloc(struct allocman *alloc, void *_vspace_pool, size_t bytes, int *error)
{
    void *ret;
    mspace_vspace_pool_t *vspace_pool = (mspace_vspace_pool_t*)_vspace_pool;
    vspace_pool->morecore_alloc = alloc;
    ret = mspace_k_r_malloc_alloc(&vspace_pool->k_r_malloc, bytes);
    vspace_pool->morecore_alloc = NULL;
    SET_ERROR(error, (ret == NULL) ? 1 : 0);
    return ret;
}

void _mspace_vspace_pool_free(struct allocman *alloc, void *_vspace_pool, void *ptr, size_t bytes)
{
    mspace_vspace_pool_t *vspace_pool = (mspace_vspace_pool_t*)_vspace_pool;
    vspace_pool->morecore_alloc = alloc;
    mspace_k_r_malloc_free(&vspace_pool->k_r_malloc, ptr);
    vspace_pool->morecore_alloc = NULL;
}

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <allocman/mspace/virtual_pool.h>
#include <allocman/allocman.h>
#include <allocman/util.h>
#include <stdlib.h>
#include <sel4/sel4.h>
#include <sel4utils/mapping.h>
#include <vka/kobject_t.h>

/* This allocator deliberately does not use the vspace library to prevent
 * circular dependencies between the vspace library and the allocator */

static int _add_page(allocman_t *alloc, seL4_CPtr pd, void *vaddr)
{
    cspacepath_t frame_path;
    uint32_t frame_cookie;
    int error;
    error = allocman_cspace_alloc(alloc, &frame_path);
    if (error) {
        return error;
    }
    frame_cookie = allocman_utspace_alloc(alloc, 12, seL4_ARCH_4KPage, &frame_path, &error);
    if (error) {
        allocman_cspace_free(alloc, &frame_path);
        return error;
    }
    error = seL4_ARCH_Page_Map(frame_path.capPtr, pd, (seL4_Word) vaddr, seL4_AllRights, 
                seL4_ARCH_Default_VMAttributes);
    if (error == seL4_FailedLookup) {
        cspacepath_t pt_path;
        uint32_t pt_cookie;
        error = allocman_cspace_alloc(alloc, &pt_path);
        if (error) {
            allocman_cspace_free(alloc, &frame_path);
            allocman_utspace_free(alloc, frame_cookie, 12);
            return error;
        }
        pt_cookie = allocman_utspace_alloc(alloc, seL4_PageTableBits, kobject_get_type(KOBJECT_PAGE_TABLE, 0), &pt_path, &error);
        if (error) {
            allocman_cspace_free(alloc, &frame_path);
            allocman_utspace_free(alloc, frame_cookie, 12);
            allocman_cspace_free(alloc, &pt_path);
            return error;
        }
        error = seL4_ARCH_PageTable_Map(pt_path.capPtr, pd, (seL4_Word) vaddr, 
                    seL4_ARCH_Default_VMAttributes);
        if (error != seL4_NoError) {
            allocman_cspace_free(alloc, &frame_path);
            allocman_utspace_free(alloc, frame_cookie, 12);
            allocman_cspace_free(alloc, &pt_path);
            allocman_utspace_free(alloc, pt_cookie, seL4_PageTableBits);
            return error;
        }
        error = seL4_ARCH_Page_Map(frame_path.capPtr, pd, (seL4_Word) vaddr, seL4_AllRights, 
                    seL4_ARCH_Default_VMAttributes);
        if (error != seL4_NoError) {
            allocman_cspace_free(alloc, &frame_path);
            allocman_utspace_free(alloc, frame_cookie, 12);
            allocman_cspace_free(alloc, &pt_path);
            allocman_utspace_free(alloc, pt_cookie, seL4_PageTableBits);
            return error;
        }
    } else if (error != seL4_NoError) {
        allocman_cspace_free(alloc, &frame_path);
        allocman_utspace_free(alloc, frame_cookie, 12);
        return error;
    }
    return 0;
}

static k_r_malloc_header_t *_morecore(uint32_t cookie, mspace_k_r_malloc_t *k_r_malloc, uint32_t new_units)
{
    uint32_t new_size;
    k_r_malloc_header_t *new_header;
    mspace_virtual_pool_t *virtual_pool = (mspace_virtual_pool_t*)cookie;
    new_size = new_units * sizeof(k_r_malloc_header_t);
    
    if (virtual_pool->pool_ptr + new_size > virtual_pool->pool_limit) {
        return NULL;
    }
    while (virtual_pool->pool_ptr + new_size > virtual_pool->pool_top) {
        int error;
        error = _add_page(virtual_pool->morecore_alloc, virtual_pool->pd, virtual_pool->pool_top);
        if (error) {
            return NULL;
        }
        virtual_pool->pool_top += PAGE_SIZE_4K;
    }
    new_header = (k_r_malloc_header_t*)virtual_pool->pool_ptr;
    virtual_pool->pool_ptr += new_size;
    return new_header;
}

void mspace_virtual_pool_create(mspace_virtual_pool_t *virtual_pool, struct mspace_virtual_pool_config config)
{
    virtual_pool->pool_ptr = config.vstart;
    virtual_pool->pool_top = virtual_pool->pool_ptr;
    virtual_pool->pool_limit = config.vstart + config.size;
    virtual_pool->morecore_alloc = NULL;
    virtual_pool->pd = config.pd;
    mspace_k_r_malloc_init(&virtual_pool->k_r_malloc, (uint32_t)virtual_pool, _morecore);
}

void *_mspace_virtual_pool_alloc(struct allocman *alloc, void *_virtual_pool, uint32_t bytes, int *error)
{
    void *ret;
    mspace_virtual_pool_t *virtual_pool = (mspace_virtual_pool_t*)_virtual_pool;
    virtual_pool->morecore_alloc = alloc;
    ret = mspace_k_r_malloc_alloc(&virtual_pool->k_r_malloc, bytes);
    virtual_pool->morecore_alloc = NULL;
    SET_ERROR(error, (ret == NULL) ? 1 : 0);
    return ret;
}

void _mspace_virtual_pool_free(struct allocman *alloc, void *_virtual_pool, void *ptr, uint32_t bytes)
{
    mspace_virtual_pool_t *virtual_pool = (mspace_virtual_pool_t*)_virtual_pool;
    virtual_pool->morecore_alloc = alloc;
    mspace_k_r_malloc_free(&virtual_pool->k_r_malloc, ptr);
    virtual_pool->morecore_alloc = NULL;
}

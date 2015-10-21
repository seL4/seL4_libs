/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <allocman/mspace/malloc.h>
#include <allocman/allocman.h>
#include <allocman/util.h>
#include <stdlib.h>

void *_mspace_malloc_alloc(allocman_t *alloc, void *unused, uint32_t bytes, int *error)
{
    void *result = malloc(bytes);
    SET_ERROR(error, result == NULL ? 1 : 0);
    return result;
}

void _mspace_malloc_free(allocman_t *alloc, void *unsued, void *ptr, uint32_t bytes)
{
    free(ptr);
}

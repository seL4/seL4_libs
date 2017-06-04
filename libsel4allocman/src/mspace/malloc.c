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

#include <allocman/mspace/malloc.h>
#include <allocman/allocman.h>
#include <allocman/util.h>
#include <stdlib.h>

void *_mspace_malloc_alloc(allocman_t *alloc, void *unused, size_t bytes, int *error)
{
    void *result = malloc(bytes);
    SET_ERROR(error, result == NULL ? 1 : 0);
    return result;
}

void _mspace_malloc_free(allocman_t *alloc, void *unsued, void *ptr, size_t bytes)
{
    free(ptr);
}

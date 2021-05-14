/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
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

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
#include <sel4/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <allocman/mspace/mspace.h>

/* Memory space allocation manager that just calls malloc.
   Malloc must either be backed by a static pool, or somehow
   call the allocation manager to get stuff to grow. Either way
   we cannot correctly report watermark information */

/* Other mspace managers might have a struct with book keeping
   information, so to maintain a consistent interface we have an
   unused parameter */
void *_mspace_malloc_alloc(struct allocman *alloc, void *unused, size_t bytes, int *error);
void _mspace_malloc_free(struct allocman *alloc, void *unsued, void *ptr, size_t bytes);

static const struct mspace_interface mspace_malloc_interface = {
    .alloc = _mspace_malloc_alloc,
    .free = _mspace_malloc_free,
    .properties = ALLOCMAN_DEFAULT_PROPERTIES,
    .mspace = NULL
};


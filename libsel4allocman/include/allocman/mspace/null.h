/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

/* Implements a null memory manager. This is useful if you are using allocators
   that do not perform any dynamic book keeping */

#include <autoconf.h>
#include <sel4/types.h>
#include <allocman/mspace/mspace.h>
#include <allocman/util.h>
#include <stdlib.h>

static inline void *_mspace_null_alloc(struct allocman *alloc, void *unused, size_t bytes, int *error)
{
    SET_ERROR(error, 1);
    return NULL;
}

static inline void _mspace_null_free(struct allocman *alloc, void *unused, void *ptr, size_t bytes)
{
    /* While this is clearly a bug, we have no nice WARN_ON or other macro for flagging this, so just ignore */
}

static const struct mspace_interface mspace_null_interface = {
    .alloc = _mspace_null_alloc,
    .free = _mspace_null_free,
    .properties = ALLOCMAN_DEFAULT_PROPERTIES,
    .mspace = NULL
};


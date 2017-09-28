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

/* Define a bunch of helper routines. To avoid conflicts with multiple potential util libraries
   always use #ifndef */

#ifndef SET_ERROR
#define SET_ERROR(var, code) do { \
    if (var) { \
        *(var) = (code); \
    } \
} while(0)
#endif

#include <sel4utils/util.h>


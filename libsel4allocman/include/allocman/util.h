/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _ALLOCMAN_UTIL_H_
#define _ALLOCMAN_UTIL_H_

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

#endif

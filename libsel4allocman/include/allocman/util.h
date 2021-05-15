/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
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


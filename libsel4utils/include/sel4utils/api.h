/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

/* This file provides various small API wrappers either for simple convenience, backwards compatibility,
   or abstracting different kernel versions */

#include <sel4/sel4.h>
#include <sel4utils/mcs_api.h>

/* Helper for generating a guard. The guard itself is a bitpacked data structure, but is passed
   to invocations as a raw word */
static inline seL4_Word api_make_guard_word(seL4_Word guard, seL4_Word guard_size) {
    /* the bitfield generated _new function will assert that our chosen values fit
       into the datastructure so there is no need for us to do anything beyond
       blindly pass them in */
    return seL4_CNode_CapData_new(guard, guard_size).words[0];
}

/* Helper for making an empty guard. Typically a guard matches zeroes and is effectively acting
   as a skip. This is a convenience wrapper and api_make_guard_word */
static inline seL4_Word api_make_guard_skip_word(seL4_Word guard_size) {
    return api_make_guard_word(0, guard_size);
}

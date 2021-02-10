/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sel4debug/stack_trace.h>

#define STACK_TRACE(x) do { \
    void *frame = __builtin_frame_address(x); \
    if (!frame) return; \
    void *ret = __builtin_return_address(x); \
    assert(ret); \
    void *addr = __builtin_extract_return_addr(ret); \
    printf("Possible stack call (%d) from %p with frame at %p\n", x, addr, frame); \
    } while (0) \
    /**/

void print_stack_trace(void) {
    STACK_TRACE(0);
    STACK_TRACE(1);
    STACK_TRACE(2);
    STACK_TRACE(3);
    STACK_TRACE(4);
    STACK_TRACE(5);
    STACK_TRACE(6);
    STACK_TRACE(7);
    STACK_TRACE(8);
    STACK_TRACE(9);
    STACK_TRACE(10);
}

/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

/* Quick and dirty way of acquiring a stack before you call a function. Use
 * this as a debugging helper when you need to call a function from a context
 * where you don't have a stack. Note, there are arguments to this function,
 * but they are passed in registers:
 *                                     ARM  x86
 *  Function pointer to jump to         r0  eax
 *  First argument                      r1  ebx
 *  Second argument                     r2  ecx
 *  Third argument                      r3  edx
 *
 * This function is not thread safe, has not been tested thoroughly and you
 * should not return from the target function being jumped to. This is *only*
 * meant as a debugging helper for when you need a stack, but not stability.
 */
void __attribute__((noreturn)) debug_trampoline(void);


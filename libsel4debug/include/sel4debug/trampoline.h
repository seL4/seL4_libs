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


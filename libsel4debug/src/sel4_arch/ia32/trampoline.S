/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* See trampoline.h for an explanation of this functionality. */

    .global debug_trampoline
    .text

debug_trampoline:
    /* Give ourselves a stack. */
    leal debug_stack_top, %esp

    /* Pass up to three arguments. */
    pushl %edx
    pushl %ecx
    pushl %ebx

    /* Woe betide the silly user if they try to return. */
    jmp *%eax

    .bss
    .align 8
    .space 4096
debug_stack_top:

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

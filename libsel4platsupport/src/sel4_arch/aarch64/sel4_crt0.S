/*
 * Copyright 2016, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(D61_BSD)
 */

#include <autoconf.h>

#ifdef CONFIG_LIB_SEL4_PLAT_SUPPORT_SEL4_START

.global _sel4_start
.extern sel4_vsyscall

.text

_sel4_start:
    /* Setup a stack for ourselves. */
    ldr     x19, =_stack_top
    mov     sp, x19 

    /* Setup bootinfo. The pointer to the bootinfo struct starts in 'r0'. */
    bl      seL4_InitBootInfo

    /* Setup stack frame ready for jumping to _start */
    /* null terminate auxv */
    mov     x0, #0
    stp     x0, x0, [sp, #-16]!
    /* give vsyscall location */
    ldr     x1, =sel4_vsyscall
    str     x1, [sp, #-8]!
    mov     x2, #32
    str     x2, [sp, #-8]!
    /* Give default page size */
    mov     x1, #4096
    str     x1, [sp, #-8]!
    mov     x2, #6
    str     x2, [sp, #-8]!
    /* Null terminate envp */
    str     x0, [sp, #-8]!
    /* Give an environment string */
    ldr     x1, =environment_string
    str     x1, [sp, #-8]!
    /* Null terminate argument vector */
    str     x0, [sp, #-8]!
    /* Give a name */
    ldr     x1, =prog_name
    str     x1, [sp, #-8]!
    /* Push argument count */
    mov     x1, #1
    str     x1, [sp, #-8]!
    /* Now go to actual _start */
    b       _start

.data
.align 16

environment_string:
.asciz "seL4=1"
prog_name:
.asciz "rootserver"

.bss
.align  8

_stack_bottom:
.space  16384
_stack_top:

#endif /* CONFIG_LIB_SEL4_PLAT_SUPPORT_SEL4_START */

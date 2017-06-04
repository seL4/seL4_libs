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
/*
 * A default seL4 crt0 for arm. It does the bare minimum required to emulate
 * a typical startup environment and jump to the regular _start symbol
 */

#include <autoconf.h>

#ifdef CONFIG_LIB_SEL4_PLAT_SUPPORT_SEL4_START

    .global _sel4_start
    .extern sel4_vsyscall

.text

_sel4_start:
    /* Setup a stack for ourselves. */
    ldr     sp, =_stack_top

    /* Construct bootinfo environment variable. The pointer to the bootinfo struct starts in 'r0'. */
    mov     r2, r0
    ldr     r0, =bootinfo_storage
    ldr     r1, =bootinfo_format
    bl      sprintf

    /* The series of pushes below will result in the final stack being 8 byte
     * aligned as it must be */
    /* Setup stack frame ready for jumping to _start */
    /* null terminate auxv */
    mov     r0, #0
    push    {r0}
    push    {r0}
    /* give vsyscall location */
    ldr     r1, =sel4_vsyscall
    push    {r1}
    mov     r2, #32
    push    {r2}
    /* Null terminate envp */
    push    {r0}
    /* Give bootinfo location */
    ldr     r1, =bootinfo_storage
    push    {r1}
    /* Set the environment to seL4 */
    ldr     r1, =environment_string
    push    {r1}
    /* Null terminate argument vector */
    push    {r0}
    /* Give a name */
    ldr     r1, =prog_name
    push    {r1}
    /* Push argument count */
    mov     r1, #1
    push    {r1}
    /* Now go to actual _start */
    ldr     pc, =_start

/* .text Literal Pool */
.pool

    .data
    .align 4

bootinfo_format:
    .asciz "bootinfo=%p"
bootinfo_storage:
    .space 21
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

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
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

    /* Setup bootinfo. The pointer to the bootinfo struct starts in 'r0'. */
    bl      seL4_InitBootInfo

    /* To ensure we end up with a correctly 8 byte aligned stack at the end we
     * need to pad by one word here */
    push    {r0}
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
    /* Give an environment string */
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

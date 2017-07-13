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
 * A default seL4 crt0 for ia32. It does the bare minimum required to emulate
 * a typical startup environment and jump to the regular _start symbol
 *
 */

#define __ASM__
#include <autoconf.h>

#ifdef CONFIG_LIB_SEL4_PLAT_SUPPORT_SEL4_START

#include <sel4/arch/constants.h>

    .global _sel4_start
    .extern sel4_vsyscall

    .text

.align 0x1000
_sel4_start:
    leaq    _stack_top, %rsp

    /* Construct bootinfo environment variable */
    movq %rdi, %rdx
    leaq bootinfo_storage, %rdi
    leaq bootinfo_format, %rsi
    call sprintf

    /* Construct a System V ABI compatible stack frame so we can go to regular _start */
    /* NULL terminate auxv */
    pushq $0
    pushq $0
    /* Give vsyscall location */
    leaq sel4_vsyscall, %rax
    pushq %rax
    pushq $32 /* AT_SYSINFO */
    /* Null terminate envp */
    pushq $0
    /* Give bootinfo location */
    leaq bootinfo_storage, %rax
    pushq %rax
    /* Set the environment to seL4 */
    leaq environment_string, %rax
    pushq %rax
    /* Null terminate argument vector */
    pushq $0
    /* Give a name for this application */
    leaq prog_name, %rax
    pushq %rax
    /* Push argument count */
    pushq $1
    /* No atexit */
    movq $0, %rdx

    /* Now go to real start function */
    jmp _start

    .data
    .align 4

bootinfo_format:
    .asciz "bootinfo=%p"
bootinfo_storage:
    .space 29
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

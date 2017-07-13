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

#include <sel4/sel4_arch/constants.h>
#include <sel4/arch/constants.h>

    .global _sel4_start
    .extern sel4_vsyscall

    .text

_sel4_start:
    leal    _stack_top, %esp

    /* Setup segment selector for IPC buffer access. */
    movw    $IPCBUF_GDT_SELECTOR, %ax
    movw    %ax, %fs

    /* Construct bootinfo environment variable */
    pushl   %ebx
    leal    bootinfo_format, %eax
    pushl   %eax
    leal    bootinfo_storage, %eax
    pushl   %eax
    call    sprintf
    addl    $12, %esp

    /* Construct a System V ABI compatible stack frame so we can go to regular _start */
    /* NULL terminate auxv */
    pushl $0
    pushl $0
    /* Give vsyscall location */
    leal sel4_vsyscall, %eax
    pushl %eax
    pushl $32 /* AT_SYSINFO */
    /* Null terminate envp */
    pushl $0
    /* Give bootinfo location */
    leal bootinfo_storage, %eax
    pushl %eax
    /* Set the environment to seL4 */
    leal environment_string, %eax
    pushl %eax
    /* Null terminate argument vector */
    pushl $0
    /* Give a name for this application */
    leal prog_name, %eax
    pushl %eax
    /* Push argument count */
    pushl $1
    /* No atexit */
    movl $0, %edx

    /* Now go to real start function */
    jmp _start

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

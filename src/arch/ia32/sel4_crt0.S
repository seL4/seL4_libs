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

#ifdef X86_64

.align 0x1000
_sel4_start:
    leaq    _stack_top, %esp

    /* Setup segment selector for IPC buffer access. */
    movw    $IPCBUF_GDT_SELECTOR, %ax
    movw    %ax, %gs

    /* Setup the global "bootinfo" structure. */
    movq    %rbx, %rdi
    call    seL4_InitBootInfo

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
    /* Give at least one environment string */
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

#else

_sel4_start:
    leal    _stack_top, %esp

    /* Setup segment selector for IPC buffer access. */
    movw    $IPCBUF_GDT_SELECTOR, %ax
    movw    %ax, %gs

    /* Setup the global "bootinfo" structure. */
    pushl   %ebx
    call    seL4_InitBootInfo
    addl    $4, %esp

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
    /* Give at least one environment string */
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

#endif

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

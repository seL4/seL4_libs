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

    /* Construct the boot_tcb_cptr environment variable */
    /* First call out to C to get the value of seL4_CapInitThreadTCB */
    call sel4ps_get_seL4_CapInitThreadTCB
    movq %rax, %rdx
    leaq boot_tid_tcb_cptr_format, %rsi
    leaq boot_tid_tcb_cptr_storage, %rdi
    call sprintf

    /* Construct a System V ABI compatible stack frame so we can go to regular _start */
    /* NULL terminate auxv */
    pushq $0
    pushq $0
    /* Give vsyscall location */
    leaq sel4_vsyscall, %rax
    pushq %rax
    pushq $32 /* AT_SYSINFO */

    /* Pass in information on our simulated TLS ELF headers, after constructing the missing pieces */
    leaq _tdata_end, %rax
    leaq _tdata_start, %rbx
    sub %rbx, %rax
    movq %rax, (tls_filesize)

    leaq _tbss_end, %rax
    subq %rbx, %rax
    movq %rax, (tls_memsize)

    leaq program_headers, %rax
    pushq %rax
    pushq $3 /* AT_PHDR */
    pushq $1 /* we have 1 header defined */
    pushq $5 /* AT_PNUM */
    pushq $56 /* 64bit elf header is 56 bytes */
    pushq $4 /* AT_PHENT */

    /* Null terminate envp */
    pushq $0
    /* Give initial tcb location */
    leaq boot_tid_tcb_cptr_storage, %rax
    pushq %rax
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
    .balign 4

bootinfo_format:
    .asciz "bootinfo=%p"
bootinfo_storage:
    .space 29
boot_tid_tcb_cptr_format:
    .asciz "boot_tcb_cptr=%p"
boot_tid_tcb_cptr_storage:
    .space 35
environment_string:
    .asciz "seL4=1"
prog_name:
    .asciz "rootserver"

program_headers:
    .4byte 7 /* p_type is PT_TLS */
    .4byte 0 /* dummy p_flags */
    .8byte 0 /* dummy p_offset */
    .8byte _tdata_start /* p_vaddr is _tdata_start */
    .8byte 0 /* dummy p_paddr */
tls_filesize:
    .8byte 0 /* p_filesz will get calculated and filled in later with _tdata_end - _tdata_start) */
tls_memsize:
    .8byte 0 /* p_memsz will get calculated and filled in later with _tbss_end - _tdata_start) */
    .8byte 8 /* p_align set to word alignment */

    .bss
    .balign  16

_stack_bottom:
    .space  16384
_stack_top:

#endif /* CONFIG_LIB_SEL4_PLAT_SUPPORT_SEL4_START */

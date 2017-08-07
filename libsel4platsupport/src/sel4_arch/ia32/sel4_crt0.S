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

    /* Construct the boot_tcb_cptr environment variable */
    /* First call out to C to get the value of seL4_CapInitThreadTCB */
    call sel4ps_get_seL4_CapInitThreadTCB
    pushl %eax
    leal boot_tid_tcb_cptr_format, %eax
    pushl %eax
    leal boot_tid_tcb_cptr_storage, %eax
    pushl %eax
    call sprintf
    addl $12, %esp

    /* Construct a System V ABI compatible stack frame so we can go to regular _start */
    /* NULL terminate auxv */
    pushl $0
    pushl $0
    /* Give vsyscall location */
    leal sel4_vsyscall, %eax
    pushl %eax
    pushl $32 /* AT_SYSINFO */

    /* Pass in information on our simulated TLS ELF headers, after constructing the missing pieces */
    leal _tdata_end, %eax
    leal _tdata_start, %ebx
    sub %ebx, %eax
    movl %eax, (tls_filesize)

    leal _tbss_end, %eax
    subl %ebx, %eax
    movl %eax, (tls_memsize)

    leal program_headers, %eax
    pushl %eax
    pushl $3 /* AT_PHDR */
    pushl $1 /* we have 1 header defined */
    pushl $5 /* AT_PNUM */
    pushl $32 /* 32bit elf header is 32 bytes */
    pushl $4 /* AT_PHENT */

    /* Null terminate envp */
    pushl $0
    /* Give initial tcb location */
    leal boot_tid_tcb_cptr_storage, %eax
    pushl %eax
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
    .balign 4

bootinfo_format:
    .asciz "bootinfo=%p"
bootinfo_storage:
    .space 21
boot_tid_tcb_cptr_format:
    .asciz "boot_tcb_cptr=%p"
boot_tid_tcb_cptr_storage:
    .space 27
environment_string:
    .asciz "seL4=1"
prog_name:
    .asciz "rootserver"

program_headers:
    .4byte 7 /* p_type is PT_TLS */
    .4byte 0 /* dummy p_offset */
    .4byte _tdata_start /* p_vaddr is _tdata_start */
    .4byte 0 /* dummy p_paddr */
tls_filesize:
    .4byte 0 /* p_filesz will get calculated and filled in later with _tdata_end - _tdata_start */
tls_memsize:
    .4byte 0 /* p_memsz will get calculated and filled in later with _tbss_end - _tdata_start */
    .4byte 0 /* dummy p_flags */
    .4byte 4 /* p_align set to word alignment */

    .bss
    .balign  16

_stack_bottom:
    .space  16384
_stack_top:

#endif /* CONFIG_LIB_SEL4_PLAT_SUPPORT_SEL4_START */

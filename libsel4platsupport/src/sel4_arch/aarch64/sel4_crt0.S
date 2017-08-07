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

#include <autoconf.h>

#ifdef CONFIG_LIB_SEL4_PLAT_SUPPORT_SEL4_START

.global _sel4_start
.extern sel4_vsyscall

.text

_sel4_start:
    /* Setup a stack for ourselves. */
    ldr     x19, =_stack_top
    mov     sp, x19

    /* Construct bootinfo environment variable */
    mov     x2, x0
    ldr     x0, =bootinfo_storage
    ldr     x1, =bootinfo_format
    bl      sprintf

    /* Construct the boot_tcb_cptr environment variable */
    /* First call out to C to get the value of seL4_CapInitThreadTCB */
    bl      sel4ps_get_seL4_CapInitThreadTCB
    mov     x2, x0
    ldr     x0, =boot_tid_tcb_cptr_storage
    ldr     x1, =boot_tid_tcb_cptr_format
    bl      sprintf

    /* Setup stack frame ready for jumping to _start */
    /* null terminate auxv */
    mov     x0, #0
    str     x0, [sp, #-8]!
    /* give vsyscall location */
    ldr     x1, =sel4_vsyscall
    str     x1, [sp, #-8]!
    mov     x2, #32
    str     x2, [sp, #-8]!
    /* Pass in information on our simulated TLS ELF headers, after constructing the missing pieces */
    ldr     x1, =_tdata_end
    ldr     x2, =_tdata_start
    sub     x1, x1, x2
    ldr     x3, =tls_filesize
    str     x1, [x3]
    ldr     x1, =_tbss_end
    sub     x1, x1, x2
    ldr     x3, =tls_memsize
    str     x1, [x3]
    ldr     x1, =program_headers
    str     x1, [sp, #-8]!
    mov     x1, #3
    str     x1, [sp, #-8]!
    mov     x1, #1
    str     x1, [sp, #-8]!
    mov     x1, #5
    str     x1, [sp, #-8]!
    mov     x1, #32
    str     x1, [sp, #-8]!
    mov     x1, #4
    str     x1, [sp, #-8]!
    /* Give default page size */
    mov     x1, #4096
    str     x1, [sp, #-8]!
    mov     x2, #6
    str     x2, [sp, #-8]!
    /* Null terminate envp */
    str     x0, [sp, #-8]!
    /* Give initial tcb location */
    ldr     x1, =boot_tid_tcb_cptr_storage
    str     x1, [sp, #-8]!
    /* Give bootinfo location */
    ldr     x1, =bootinfo_storage
    str     x1, [sp, #-8]!
    /* Set the environment to seL4 */
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
.balign 16

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

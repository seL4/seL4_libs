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

    /* Construct the boot_tcb_cptr environment variable */
    /* First call out to C to get the value of seL4_CapInitThreadTCB */
    bl      sel4ps_get_seL4_CapInitThreadTCB
    mov     r2, r0
    ldr     r0, =boot_tid_tcb_cptr_storage
    ldr     r1, =boot_tid_tcb_cptr_format
    bl      sprintf

    /* The series of pushes below will result in the final stack being 4 byte
     * aligned, instead of 8 byte, as it must be, so perform a dummy extra push here */
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
    /* Pass in information on our simulated TLS ELF headers, after constructing the missing pieces */
    ldr     r1, =_tdata_end
    ldr     r2, =_tdata_start
    sub     r1, r1, r2
    ldr     r3, =tls_filesize
    str     r1, [r3]
    ldr     r1, =_tbss_end
    sub     r1, r1, r2
    ldr     r3, =tls_memsize
    str     r1, [r3]
    ldr     r1, =program_headers
    push    {r1}
    mov     r1, #3
    push    {r1} /* AT_PHDR */
    mov     r1, #1
    push    {r1}
    mov     r1, #5
    push    {r1} /* AT_PNUM */
    mov     r1, #32
    push    {r1}
    mov     r1, #4
    push    {r1} /* AT_PHENT */
    /* Null terminate envp */
    push    {r0}
    /* Give initial tcb location */
    ldr     r1, =boot_tid_tcb_cptr_storage
    push    {r1}
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
    .balign  8

_stack_bottom:
    .space  16384
_stack_top:

#endif /* CONFIG_LIB_SEL4_PLAT_SUPPORT_SEL4_START */

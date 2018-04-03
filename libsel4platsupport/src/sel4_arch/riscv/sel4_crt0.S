/*
 * Copyright 2018, Data61
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
#if __riscv_xlen == 32
#define LW lw
#define SW sw
#define REGSIZE 4
#else
#define LW ld
#define SW sd
#define REGSIZE 8
#endif

    .global _sel4_start
    .extern sel4_vsyscall
    .extern __dso_handle
.text

_sel4_start:

/* Set gp for relaxation. See
 * https://www.sifive.com/blog/2017/08/28/all-aboard-part-3-linker-relaxation-in-riscv-toolchain/
 */
.option push
.option norelax
1:auipc gp, %pcrel_hi(__global_pointer$)
  addi  gp, gp, %pcrel_lo(1b)
.option pop

    /* Setup a stack for ourselves. */
    la     sp, _stack_top

    /* Setup bootinfo. The pointer to the bootinfo struct starts in 'a0'. */
    //jal      seL4_InitBootInfo
    /* Construct bootinfo environment variable */
    mv     a2, a0
    la     a0, bootinfo_storage
    la     a1, bootinfo_format
    jal    sprintf


    /* Setup stack frame ready for jumping to _start */
    addi sp, sp, -(10*REGSIZE)

    /* argc */
    li   a1, 1
    SW   a1, 0(sp)

    /* prog name */
    la   a1, prog_name
    SW   a1, (1*REGSIZE)(sp)

    /* Null-terminate arg vector */
    SW   x0, (2*REGSIZE)(sp)

    /* Give an environment string */
    la   a1, environment_string
    SW   a1, (3*REGSIZE)(sp)

    /* Give bootinfo location */
    la   a1, bootinfo_storage
    SW   a1, (4*REGSIZE)(sp)

    /* Null terminate envp */
    SW   x0, (5*REGSIZE)(sp)

    /* give vsyscall location */
    li   a1, 32
    SW   a1, (6*REGSIZE)(sp)
    la   a1, sel4_vsyscall
    SW   a1, (7*REGSIZE)(sp)

    /* null terminate auxv */
    SW   x0, (8*REGSIZE)(sp)
    SW   x0, (9*REGSIZE)(sp)
    SW   x0, (10*REGSIZE)(sp)

    /* Now go to actual _start */
    j       _start

/* .text Literal Pool */
    .data
    .align  8
bootinfo_format:
    .asciz "bootinfo=%p"
    .align  8
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

/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
/*
 * A default crt0 for ia32. It does the bare minimum required to get into
 * main() with a sensible C environment.
 *
 * This file will only be linked in if:
 *   - no other file already provides a _start symbol, and
 *   - _start is an undefined external symbol (force this by passing
 *     "-u _start" to ld).
 */

#define __ASM__
#include <autoconf.h>
#include <sel4platsupport/gen_config.h>

#ifdef CONFIG_LIB_SEL4_PLAT_SUPPORT_START

#include <sel4/arch/constants.h>

    .global _start

    .text

.align 0x1000
_start:
    leaq    _stack_top, %rsp

#   setup the bootinfo
    movq    %rbx, %rdi
    call    seL4_InitBootInfo

    /* Call constructors and other initialisation functions. */
    call    _init

    call    main
# the return value is in rax
    movq    %rax, %rdi
    call    exit

1:  jmp     1b

    .bss
    .balign  8

_stack_bottom:
    .space  16384
_stack_top:

#endif /* CONFIG_LIB_SEL4_PLAT_SUPPORT_START */

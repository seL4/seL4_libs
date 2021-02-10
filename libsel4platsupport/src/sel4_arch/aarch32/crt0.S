/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
/*
 * A default crt0 for ARM. It does the bare minimum required to get into
 * main() with a sensible C environment.
 *
 * This file will only be linked in if:
 *   - no other file already provides a _start symbol, and
 *   - _start is an undefined external symbol (force this by passing
 *     "-u _start" to ld).
 */

#include <autoconf.h>
#include <sel4platsupport/gen_config.h>

#ifdef CONFIG_LIB_SEL4_PLAT_SUPPORT_START

.text

/*
 * Image Entry point.
 */
.global _start
_start:
    /* Setup a stack for ourselves. */
    ldr     sp, =_stack_top

    /* Setup bootinfo. The pointer to the bootinfo struct starts in 'r0'. */
    bl      seL4_InitBootInfo

    /* Call constructors and other initialisation functions. */
    bl      _init

    /* Call main. */
    bl      main
    b       exit

/* .text Literal Pool */
.pool

/* Stack for the image. */
.bss
.balign  8
_stack_bottom:
.space  16384
_stack_top:

#endif /* CONFIG_LIB_SEL4_PLAT_SUPPORT_START */

/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
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
    ldr     x19, =_stack_top
    mov     sp, x19

    /* Setup bootinfo. The pointer to the bootinfo struct starts in 'r0'. */
    bl      seL4_InitBootInfo

    /* Call constructors and other initialisation functions. */
    bl      _init

    /* Call main. */
    bl      main
    b       exit

/* Stack for the image. */
.bss
.balign  8
_stack_bottom:
.space  16384
_stack_top:

#endif /* CONFIG_LIB_SEL4_PLAT_SUPPORT_START */

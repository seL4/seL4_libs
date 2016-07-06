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
 * A default crt0 for ARM. It does the bare minimum required to get into
 * main() with a sensible C environment.
 *
 * This file will only be linked in if:
 *   - no other file already provides a _start symbol, and
 *   - _start is an undefined external symbol (force this by passing
 *     "-u _start" to ld).
 */

#include <autoconf.h>

#ifdef CONFIG_LIB_SEL4_PLAT_SUPPORT_START
.extern __stdout_used
.text

/*
 * Image Entry point.
 */
.global _start
_start:
    /* Dirty hack to load gp and avoid load/store exceptions, this should be
    * defined by the linker */
#error "you know"
     li gp, 0x800
     la s0, __stdout_used
     add gp, gp, s0
    /* Setup a stack for ourselves. */
    la     sp, _stack_top

    /* Setup bootinfo. The pointer to the bootinfo struct starts in 'a0'. */
    #add a0, x0, a5
    jal      seL4_InitBootInfo

    la      a1, sel4_vsyscall
    /* Call constructors and other initialisation functions. */
    jal      _init

    /* Call main. */
    jal     main
    j       exit

/* .text Literal Pool */

/* Stack for the image. */
.bss
.align  8
_stack_bottom:
.space  16384
_stack_top:

#endif /* CONFIG_LIB_SEL4_PLAT_SUPPORT_START */

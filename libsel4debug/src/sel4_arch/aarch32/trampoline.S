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

/* See trampoline.h for an explanation of this functionality. */

    .global debug_trampoline
    .text

#ifdef CONFIG_ARCH_AARCH32
debug_trampoline:
    /* Give ourselves a stack. */
    ldr sp, =debug_stack_top

    /* Shuffle the (up to) three arguments. */
    mov r4, r0
    mov r0, r1
    mov r1, r2
    mov r2, r3

    /* bl so, if the caller had set the lr, the callee can return through to
     * it.
     */
    blx r4
    bx lr

    .bss
    .align  8
    .space  4096
debug_stack_top:
#endif /* CONFIG_ARCH_AARCH64 */

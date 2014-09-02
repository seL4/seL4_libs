/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _LIBSEL4_DEBUG_ARCH_DEBUG_
#define _LIBSEL4_DEBUG_ARCH_DEBUG_

/* An extremely sketchy, last-ditch method of getting a register dump. If you
 * have any other option available, please use it instead.
 */

#include <sel4/types.h>
#include <stdio.h>

/* Print out the current register set of the caller. This function does not
 * assume the caller has a valid stack when they invoke it.
 *
 * Note, that it is forced inline so it has the register set at the location of
 * the call, not after caller-register-save has been done.
 */
static inline void debug_dump_registers(void)
__attribute__((always_inline));

/* WARNING: This function has not been extensively tested and may not work in
 * all scenarios. Where possible, you should avoid using this function and use
 * a second thread to retrieve your register set.
 *
 * If you enter this function in a thumb code context everything will go pear-
 * shaped, but seL4 has no support for Thumb currently anyway.
 */
static inline void debug_dump_registers(void)
{
    /* A variable to store the current register set in. No reason this needs to
     * be a seL4_UserContext, but it's a convenient struct to store registers
     * in. This is marked static to ensure it doesn't get allocated on the
     * stack (which may not be valid at this point). Technically the compiler
     * may allocate stack space for it, but there is no reason for it to do so.
     * The fact that the value of this variable is maintained across function
     * calls (by virtue of being static) is a useless side-effect in this case.
     */
    static seL4_UserContext *context;

    asm volatile (
        /* Assume that the caller enters this function with an invalid or corrupt
         * stack. We want to stash the stack pointer somewhere and then reassign it
         * to a valid stack, but we can't reference anything sensible without
         * losing the current SP, stomping on a register (we assume all the
         * caller's registers need to be maintained) or assuming a particular
         * address will contain a valid stack (we can't assume this because we
         * don't know how the program we're executing in was linked).
         *
         * To dodge all of this, provide an inline stack right here. By inspecting
         * the output, the printf call in this function needs 13 words of stack
         * space so let's give it 20 just to be safe. Conveniently the register
         * width on ARM is the same as the instruction width so we can just provide
         * NOPs to measure this out. This doesn't affect the control flow because
         * execution drops straight through the "stack" into the following code.
         *
         * Note, all of this will break in the face of read-only code mappings, but
         * ARM ELF files never seem to do this which is fortunate.
         */
        "\tnop                  \n"
        "\tnop                  \n"
        "\tnop                  \n"
        "\tnop                  \n"
        "\tnop                  \n"
        "\tnop                  \n"
        "\tnop                  \n"
        "\tnop                  \n"
        "\tnop                  \n"
        "\tnop                  \n"
        "\tnop                  \n"
        "\tnop                  \n"
        "\tnop                  \n"
        "\tnop                  \n"
        "\tnop                  \n"
        "\tnop                  \n"
        "\tnop                  \n"
        "\tnop                  \n"
        "\tnop                  \n"
        "\tnop                  \n"

        /* Space on the temporary stack to store the register set... */
        "\tnop                  \n" /*    ^    */
        "\tnop                  \n" /*    |    */
        "\tnop                  \n" /*    |    */
        "\tnop                  \n" /*    |    */
        "\tnop                  \n" /*    |    */
        "\tnop                  \n" /*    |    */
        "\tnop                  \n" /*    |    */
        "\tnop                  \n" /*    |    */
        "\tnop                  \n" /* context */
        "\tnop                  \n" /*    |    */
        "\tnop                  \n" /*    |    */
        "\tnop                  \n" /*    |    */
        "\tnop                  \n" /*    |    */
        "\tnop                  \n" /*    |    */
        "\tnop                  \n" /*    |    */
        "\tnop                  \n" /*    |    */
        "\tnop                  \n" /*    v    */

        /* ...and one word to stash the original SP. */
        "\tnop                  \n"

        /* Save the original SP in the preceeding word so we can retrieve it later.
         * According to the ARM docs this method of addressing (PC-relative offset)
         * is illegal, but it works so...
         */
        "\tstr sp, [pc, #-12]   \n"

        /* Now switch to our temporary safe stack. */
        "\tsub sp, pc, #16      \n"

        /* Push all the registers onto the stack in the reverse order of the
         * layout of seL4_UserContext.
         */
        "\tpush {r14}           \n"
        "\tpush {r7}            \n"
        "\tpush {r6}            \n"
        "\tpush {r5}            \n"
        "\tpush {r4}            \n"
        "\tpush {r3}            \n"
        "\tpush {r2}            \n"
        "\tpush {r12}           \n"
        "\tpush {r11}           \n"
        "\tpush {r10}           \n"
        "\tpush {r9}            \n"
        "\tpush {r8}            \n"
        "\tpush {r1}            \n"
        "\tpush {r0}            \n"

        /* CPSR is a bit more complicated because we can't push it directly. Note
         * that this stomps on r0 so we'll need to restore it later.
         *
         * FIXME: We've stomped on the APSR flags, but it doesn't have to be this
         * way. We could stash the original CPSR (like the SP) before doing any
         * arithmetic.
         */
        "\tmrs r0, cpsr         \n"
        "\tpush {r0}            \n"

        /* Push the ORIGINAL SP (addressed from the current SP). */
        "\tldr r0, [sp, #60]    \n"
        "\tpush {r0}            \n"

        /* Push the originating PC. Calculated by the length of the
         * instruction stream (including the initial NOP slide) subtracted from the
         * current PC with some adjustment for pipeline effects.
         */
        "\tsub r0, pc, #240    \n"
        "\tpush {r0}           \n"
        :
        :
        :"sp" /* We clobbered r0 as well, but we don't want GCC trying to save
              * it for us. GCC can't actually do anything sensible about this
              * clobber anyway.
              */
    );

    /* Assign context to point at the seL4_UserContext we have just constructed
     * on the temporary stack. We can't do this in the previous asm block
     * because this clobbers an unknown (compiler-assigned) register. The
     * compiler could actually introduce something in between the two asm
     * blocks, but in practice it has no reason to and doesn't.
     */
    asm volatile (
        "\tmov %0, sp          \n"
        :"=r"(context)
        :
        :
    );

    printf(
        "Register dump:\n"
        " pc   = %p\n"
        " sp   = %p\n"
        " cpsr = %p\n"
        " r0   = %p\n"
        " r1   = %p\n"
        " r2   = %p\n"
        " r3   = %p\n"
        " r4   = %p\n"
        " r5   = %p\n"
        " r6   = %p\n"
        " r7   = %p\n"
        " r8   = %p\n"
        " r9   = %p\n"
        " r10  = %p\n"
        " r11  = %p\n"
        " r12  = %p\n"
        " r14  = %p\n",
        (void*)context->regs.pc,
        (void*)context->regs.sp,
        (void*)context->regs.cpsr,
        (void*)context->regs.r0,
        (void*)context->regs.r1,
        (void*)context->regs.r2,
        (void*)context->regs.r3,
        (void*)context->regs.r4,
        (void*)context->regs.r5,
        (void*)context->regs.r6,
        (void*)context->regs.r7,
        (void*)context->regs.r8,
        (void*)context->regs.r9,
        (void*)context->regs.r10,
        (void*)context->regs.r11,
        (void*)context->regs.r12,
        (void*)context->regs.r14
    );

    asm volatile (
        /* Restore r0. */
        "\tmov r0, %0           \n"
        /* Restore the SP. */
        "\tldr sp, [sp, #68]    \n"
        :
        :"r"(context->regs.r0) /* FIXME: The compiler probably uses another register here that it will attempt to save on the stack (which we are about to rip out) if it can't find a dead one. */
        :"r0", "sp"
    );

    /* Note that we just completely destroyed the NOP slide at the start of the
     * function and made no attempt to undo this. We don't need to because this
     * function is always inlined so at each call site it gets a (brand-new)
     * block of NOPs.
     *
     * FIXME: This logic doesn't work at all if the function is called from any
     * location that gets executed more than once. We probably SHOULD restore
     * the NOPs.
     */
}

#endif /* !_LIBSEL4_DEBUG_ARCH_DEBUG_ */

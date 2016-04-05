/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef _SEL4UTILS_SEL4_ARCH_UTIL_H
#define _SEL4UTILS_SEL4_ARCH_UTIL_H

#include <stdint.h>
#include <sel4/sel4.h>
#include <sel4/arch/pfIPC.h>
#include <sel4/arch/exIPC.h>

#define EXCEPT_IPC_SYS_MR_IP EXCEPT_IPC_SYS_MR_RIP
#define ARCH_SYSCALL_INSTRUCTION_SIZE 2

static inline void
sel4utils_set_instruction_pointer(seL4_UserContext *regs, seL4_Word value)
{
    regs->rip = value;
}

static inline seL4_Word
sel4utils_get_instruction_pointer(seL4_UserContext regs)
{
    return regs.rip;
}

static inline seL4_Word
sel4utils_get_sp(seL4_UserContext regs)
{
    return regs.rsp;
}

static inline void
sel4utils_set_stack_pointer(seL4_UserContext *regs, seL4_Word value)
{
    regs->rsp = value;
}

#endif /* _SEL4UTILS_SEL4_ARCH_UTIL_H */

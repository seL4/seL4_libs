/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef _SEL4UTILS_ARCH_UTIL_H
#define _SEL4UTILS_ARCH_UTIL_H

#include <sel4/sel4.h>
#include <sel4/arch/pfIPC.h>
#include <sel4/arch/exIPC.h>

#define EXCEPT_IPC_SYS_MR_IP EXCEPT_IPC_SYS_MR_EIP


static inline int
sel4utils_is_read_fault(void)
{
    seL4_Word fsr = seL4_GetMR(SEL4_PFIPC_FSR);
    return (fsr & (1 << 1)) == 0;
}


static inline void 
sel4utils_set_instruction_pointer(seL4_UserContext *regs, seL4_Word value) 
{
    regs->eip = value;
}

static inline seL4_Word
sel4utils_get_instruction_pointer(seL4_UserContext regs) 
{
    return regs.eip;
}

static inline void
sel4utils_set_stack_pointer(seL4_UserContext *regs, seL4_Word value)
{
    regs->esp = value;
}

static inline void
sel4utils_set_arg0(seL4_UserContext *regs, seL4_Word value)
{
    regs->eip = value;
}
#endif /* _SEL4UTILS_ARCH_UTIL_H */



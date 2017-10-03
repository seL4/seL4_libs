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
#pragma once

#include <stdint.h>
#include <sel4/sel4.h>
#include <sel4/arch/pfIPC.h>
#include <sel4/arch/exIPC.h>

#define EXCEPT_IPC_SYS_MR_IP EXCEPT_IPC_SYS_MR_RIP

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


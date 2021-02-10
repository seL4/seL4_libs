/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <stdint.h>
#include <sel4/sel4.h>
#include <sel4/faults.h>

#define EXCEPT_IPC_SYS_MR_IP EXCEPT_IPC_SYS_MR_EIP

static inline void sel4utils_set_instruction_pointer(seL4_UserContext *regs, seL4_Word value)
{
    regs->eip = value;
}

static inline seL4_Word sel4utils_get_instruction_pointer(seL4_UserContext regs)
{
    return regs.eip;
}

static inline seL4_Word sel4utils_get_sp(seL4_UserContext regs)
{
    return regs.esp;
}

static inline void sel4utils_set_stack_pointer(seL4_UserContext *regs, seL4_Word value)
{
    regs->esp = value;
}

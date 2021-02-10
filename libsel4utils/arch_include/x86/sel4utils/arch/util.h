/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <stdint.h>
#include <sel4/sel4.h>
#include <sel4/faults.h>
#include <sel4utils/sel4_arch/util.h>
#include <utils/arith.h>

#define ARCH_SYSCALL_INSTRUCTION_SIZE 2

static inline int
sel4utils_is_read_fault(void)
{
    seL4_Word fsr = seL4_GetMR(seL4_VMFault_FSR);
    return (fsr & (BIT(1))) == 0;
}


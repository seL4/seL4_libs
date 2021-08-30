/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sel4/sel4.h>
#include <sel4debug/register_dump.h>

#include <stdbool.h>
#include <stdio.h>
#include <utils/zf_log.h>

void sel4debug_dump_registers(seL4_CPtr tcb)
{
    sel4debug_dump_registers_prefix(tcb, "");
}

void sel4debug_dump_registers_prefix(seL4_CPtr tcb, char *prefix)
{
    seL4_UserContext context; /* basically an array of seL4_Word */
    const int num_regs = sizeof(context) / sizeof(seL4_Word);
    int error = seL4_TCB_ReadRegisters(tcb, false, 0, num_regs, &context);
    if (error) {
        ZF_LOGE("Failed to read registers for tcb 0x%"SEL4_PRIx_word", error %d",
                tcb, error);
        return;
    }

    printf("%sRegister dump:\n", prefix);
    for (unsigned int i = 0; i < num_regs; i++) {
        printf("%s%s:\t0x%"SEL4_PRIx_word"\n",
               prefix, register_names[i], ((seL4_Word *)&context)[i]);
    }
}

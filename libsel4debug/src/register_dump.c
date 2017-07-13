/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 *  @LICENSE(DATA61_BSD)
 */

#include <sel4/sel4.h>
#include <sel4debug/register_dump.h>

#include <stdbool.h>
#include <stdio.h>
#include <utils/zf_log.h>

void
sel4debug_dump_registers(seL4_CPtr tcb)
{
    seL4_UserContext context;
    int error;
    const int num_regs = sizeof(context) / sizeof(seL4_Word);

    error = seL4_TCB_ReadRegisters(tcb, false, 0, num_regs, &context);
    if (error) {
        ZF_LOGE("Failed to read registers for tcb 0x%lx, error %d", (long) tcb, error);
        return;
    }

    printf("Register dump:\n");
    for (int i = 0; i < num_regs; i++) {
        printf("%s\t:0x%lx\n", register_names[i], (long) ((seL4_Word * )&context)[i]);
    }
}

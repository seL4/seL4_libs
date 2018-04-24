/*
 * Copyright 2018, Data61
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

#include <autoconf.h>

#include <assert.h>
#include <sel4/sel4.h>
#include <stdlib.h>
#include <utils/util.h>
#include <vka/cspacepath_t.h>

/**
 * Request a cap to a specific IRQ number on the system
 *
 * @param irq the irq number to get the cap for
 * @param data cookie for the underlying implementation
 * @param the CNode in which to put this cap
 * @param the index within the CNode to put cap
 * @param Depth of index
 */
typedef seL4_Error (*arch_simple_get_IRQ_control_fn)(void *data, int irq, seL4_CNode cnode, seL4_Word index, uint8_t depth); 

typedef struct arch_simple {
    void *data;
    arch_simple_get_IRQ_control_fn irq;
} arch_simple_t;

static inline seL4_Error
arch_simple_get_IOPort_cap(UNUSED arch_simple_t *simple, UNUSED uint16_t start_port,
                           UNUSED uint16_t end_port, UNUSED seL4_Word root, UNUSED seL4_Word dest, UNUSED seL4_Word depth) {
    ZF_LOGF("Calling get_IOPort_cap on arm!");
    return seL4_IllegalOperation;
}

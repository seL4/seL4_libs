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

#include <autoconf.h>

#include <assert.h>
#include <sel4/sel4.h>
#include <stdlib.h>
#include <utils/util.h>
#include <vka/cspacepath_t.h>

/* Request a cap to a specific IRQ number on the system
 *
 * @param irq the irq number to get the cap for
 * @param data cookie for the underlying implementation
 * @param the CNode in which to put this cap
 * @param the index within the CNode to put cap
 * @param Depth of index
 */
typedef seL4_Error (*simple_get_IRQ_handler_fn)(void *data, int irq, seL4_CNode cnode, seL4_Word index, uint8_t depth);

typedef seL4_Error (*simple_get_iospace_cap_count_fn)(void *data, int *count);
typedef seL4_CPtr  (*simple_get_nth_iospace_cap_fn)(void *data, int n);

typedef struct arch_simple {
    simple_get_IRQ_handler_fn irq;
#ifdef CONFIG_ARM_SMMU
    simple_get_iospace_cap_count_fn iospace_cap_count;
    simple_get_nth_iospace_cap_fn   iospace_get_nth_cap;
#endif
    void *data;
} arch_simple_t;

static inline seL4_CPtr
arch_simple_get_IOPort_cap(UNUSED arch_simple_t *simple, UNUSED uint16_t start_port,
                           UNUSED uint16_t end_port) {
    ZF_LOGF("Calling get_IOPort_cap on arm!");
    return -1;
}


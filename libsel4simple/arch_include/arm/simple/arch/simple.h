/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <autoconf.h>

#include <assert.h>
#include <sel4/sel4.h>
#include <stdlib.h>
#include <utils/util.h>
#include <vka/cspacepath_t.h>

/* Simple does not address initial null caps, including seL4_CapNull.
 * seL4_CapIOSpace, seL4_CapIOPortControl are null on architectures other than x86 */
#define SIMPLE_SKIPPED_INIT_CAPS 3

/* Request a cap to a specific IRQ number on the system
 *
 * @param irq the irq number to get the cap for
 * @param data cookie for the underlying implementation
 * @param the CNode in which to put this cap
 * @param the index within the CNode to put cap
 * @param Depth of index
 */
typedef seL4_Error(*simple_get_IRQ_handler_fn)(void *data, int irq, seL4_CNode cnode,
                                               seL4_Word index, uint8_t depth);
typedef seL4_Error(*simple_get_IRQ_trigger_fn)(void *data, int irq, int trigger, int core, seL4_CNode root,
                                               seL4_Word index, uint8_t depth);

typedef seL4_Error(*simple_get_iospace_cap_count_fn)(void *data, int *count);
typedef seL4_CPtr(*simple_get_nth_iospace_cap_fn)(void *data, int n);

typedef struct arch_simple {
    simple_get_IRQ_handler_fn irq;
    simple_get_IRQ_trigger_fn irq_trigger;
#ifdef CONFIG_TK1_SMMU
    simple_get_iospace_cap_count_fn iospace_cap_count;
    simple_get_nth_iospace_cap_fn   iospace_get_nth_cap;
#endif
    void *data;
} arch_simple_t;

static inline seL4_Error arch_simple_get_IOPort_cap(UNUSED arch_simple_t *simple, UNUSED uint16_t start_port,
                                                    UNUSED uint16_t end_port, UNUSED seL4_Word root,
                                                    UNUSED seL4_Word dest, UNUSED seL4_Word depth)
{
    ZF_LOGF("Calling get_IOPort_cap on arm!");
    return seL4_IllegalOperation;
}

static inline seL4_Error arch_simple_get_IRQ_trigger(arch_simple_t *simple, int irq, int trigger,
                                                     cspacepath_t path)
{
    if (!simple) {
        ZF_LOGE("Simple is NULL");
        return seL4_InvalidArgument;
    }
    if (!simple->irq_trigger) {
        ZF_LOGE("Not implemented");
        return seL4_InvalidArgument;
    }

    return simple->irq_trigger(simple->data, irq, trigger, 0, path.root, path.capPtr, path.capDepth);
}

static inline seL4_Error arch_simple_get_IRQ_trigger_cpu(arch_simple_t *simple, int irq, int trigger, int core,
                                                         cspacepath_t path)
{
    if (!simple) {
        ZF_LOGE("Simple is NULL");
        return seL4_InvalidArgument;
    }
    if (!simple->irq_trigger) {
        ZF_LOGE("Not implemented");
        return seL4_InvalidArgument;
    }

    return simple->irq_trigger(simple->data, irq, trigger, core, path.root, path.capPtr, path.capDepth);
}

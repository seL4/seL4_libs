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

#include <autoconf.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <sel4/sel4.h>

#include <simple-default/simple-default.h>

#include <vspace/page.h>

seL4_Error simple_default_get_irq(void *data, int irq, seL4_CNode root, seL4_Word index, uint8_t depth) {
    return seL4_IRQControl_Get(seL4_CapIRQControl, irq, root, index, depth);
}

#ifdef CONFIG_ARM_SMMU
seL4_Error simple_default_get_iospace_cap_count(void *data, int *count)
{
    seL4_BootInfo *bi = data;
    seL4_SlotRegion reg;

    if (!bi) {
        ZF_LOGE("BootInfo pointer is NULL");
        *count = 0;
        return seL4_InvalidArgument;
    }

    reg = bi->ioSpaceCaps;

    if (reg.end <= reg.start) {
        ZF_LOGE("BootInfo for iospace caps is incorrect");
        *count = 0;
        return seL4_InvalidArgument;
    }

    *count = reg.end - reg.start;
    return seL4_NoError;
}

seL4_CPtr simple_default_get_iospace_nth_cap(void *data, int n)
{
    seL4_BootInfo *bi = data;
    seL4_SlotRegion reg;
    seL4_CPtr cap;

    if (!bi) {
        ZF_LOGE("BootInfo pointer is NULL");
        return seL4_CapNull;
    }

    reg = bi->ioSpaceCaps;

    if (reg.end <= reg.start) {
        ZF_LOGE("BootInfo for iospace caps is incorrect");
        return seL4_CapNull;
    }

    cap = reg.start;
    cap += n;

    if (cap >= reg.end || cap < reg.start) {
        ZF_LOGE("Invalid IOSpace cap");
        return seL4_CapNull;
    }

    return cap;
}

#endif

void
simple_default_init_arch_simple(arch_simple_t *simple, void *data)
{
    simple->data = data;
    simple->irq = simple_default_get_irq;
#ifdef CONFIG_ARM_SMMU
    simple->iospace_cap_count = simple_default_get_iospace_cap_count;
    simple->iospace_get_nth_cap = simple_default_get_iospace_nth_cap;
#endif
}

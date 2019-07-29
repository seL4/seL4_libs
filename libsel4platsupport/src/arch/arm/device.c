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

#include <sel4platsupport/device.h>
#include <utils/util.h>

int sel4platsupport_arch_copy_irq_cap(arch_simple_t *arch_simple, ps_irq_t *irq, cspacepath_t *dest)
{
    switch (irq->type) {
    case PS_TRIGGER:
        return arch_simple_get_IRQ_trigger(arch_simple, irq->trigger.number, irq->trigger.trigger, *dest);
    case PS_PER_CPU:
        return arch_simple_get_IRQ_trigger_cpu(arch_simple, irq->cpu.number, irq->cpu.trigger, irq->cpu.cpu_idx, *dest);
    default:
        ZF_LOGE("unknown irq type");
        return -1;
    }
}

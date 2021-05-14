/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
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

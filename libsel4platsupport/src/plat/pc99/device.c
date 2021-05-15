/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <autoconf.h>
#include <sel4platsupport/gen_config.h>
#include <sel4platsupport/device.h>
#include <sel4platsupport/platsupport.h>
#include <utils/util.h>
#include <platsupport/irq.h>

int sel4platsupport_arch_copy_irq_cap(arch_simple_t *arch_simple, ps_irq_t *irq, cspacepath_t *dest)
{
    switch (irq->type) {
    case PS_MSI:
        return arch_simple_get_msi(arch_simple, *dest, irq->msi.pci_bus, irq->msi.pci_dev,
                                   irq->msi.pci_func, irq->msi.handle, irq->msi.vector);
    case PS_IOAPIC:
        return arch_simple_get_ioapic(arch_simple, *dest, irq->ioapic.ioapic, irq->ioapic.pin,
                                      irq->ioapic.level, irq->ioapic.polarity,
                                      irq->ioapic.vector);
    default:
        ZF_LOGE("unknown irq type");
        return -1;
    }
}

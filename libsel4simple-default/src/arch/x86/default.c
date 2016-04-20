/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <autoconf.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <sel4/sel4.h>

#include <simple-default/simple-default.h>

#include <vspace/page.h>

seL4_Error simple_default_get_irq(void *data, int irq, seL4_CNode root, seL4_Word index, uint8_t depth) {
#ifdef CONFIG_IRQ_IOAPIC
    /* we need to try and guess how to map a requested IRQ to an IOAPIC
     * pin, as well as what the edge and polarity detection mode is.
     * Without any way to inspect the ACPI tables here we make the following
     * assumptions
     *   + There is an override for ISA-IRQ-0 -> GSI 2
     *   + Only one IOAPIC and error on IRQs >= 24
     *   + IRQs below 16 are ISA and edge detected polarity high
     *   + IRQs 16 and above are PCI and level detected polarity low
     */
    int vector = irq;
    if (irq == 0) {
        irq = 2;
    }
    int level;
    int low_polarity;
    if (irq >= 16) {
        level = 1;
        low_polarity = 1;
    } else {
        level = 0;
        low_polarity = 0;
    }
    return seL4_IRQControl_GetIOAPIC(seL4_CapIRQControl, root, index, depth, 0, irq, level, low_polarity, vector);
#else
    return seL4_IRQControl_Get(seL4_CapIRQControl, irq, root, index, depth);
#endif
}

seL4_Error
simple_default_get_msi(void *data, int irq, seL4_CNode root, seL4_Word index, uint8_t depth) 
{
    return seL4_IRQControl_GetMSI(seL4_CapIRQControl, root, index, depth, 0, 0, 0, 0, irq);
}

seL4_CPtr 
simple_default_get_IOPort_cap(void *data, uint16_t start_port, uint16_t end_port) {
    return seL4_CapIOPort;
}

void
simple_default_init_arch_simple(arch_simple_t *simple, void *data) 
{
    simple->data = data;
    simple->msi = simple_default_get_msi;
    simple->irq = simple_default_get_irq;
    simple->IOPort_cap = simple_default_get_IOPort_cap;
}


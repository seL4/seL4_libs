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

#ifndef __SEL4PLATSUPPORT_ARCH_DEVICE_H__
#define __SEL4PLATSUPPORT_ARCH_DEVICE_H__

#include <sel4/sel4.h>
#include <sel4platsupport/arch/device.h>
#include <simple/simple.h>
#include <vka/vka.h>

/*
 * Allocate a cslot for an msi irq and get the cap for an irq.

 * @param vka to allocate slot with
 * @param simple to get the cap from
 * @param irq_number to get the cap to
 * @param[out] dest empty path struct to return path to irq in
 * @return 0 on success
 */
seL4_Error sel4platsupport_copy_msi_cap(vka_t *vka, simple_t *simple, seL4_Word irq_number,
                                        cspacepath_t *dest);

/*
 * Allocate a cslot for an ioapic irq and get the cap for it
 *
 * @param vka to allocate slot with
 * @param simple to get the cap from
 * @param ioapic index of physical ioapic of interrupt source
 * @param pin number of the IRQ on the specified physical IOAPIC
 * @param level IRQ is level or edge triggered
 * @param polarity IRQ is active high or active low trigered
 * @param vector Vector on the CPU to allocate for interrupt delivery
 * @param[out] dest empty path struct to return path to irq in
 * @return 0 on success
 */
seL4_Error
sel4platsupport_copy_ioapic_cap(vka_t *vka, simple_t *simple, seL4_Word ioapic, seL4_Word pin, seL4_Word level,
                                seL4_Word polarity, seL4_Word vector, cspacepath_t *dest);

#endif /* __SEL4PLATSUPPORT_ARCH_DEVICE_H__ */

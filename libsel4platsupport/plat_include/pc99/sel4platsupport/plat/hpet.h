/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __SEL4_PLATSUPPORT_HPET_H
#define __SEL4_PLATSUPPORT_HPET_H

#include <platsupport/plat/acpi/acpi.h>
#include <platsupport/timer.h>

#include <simple/simple.h>

#include <sel4/types.h>

#include <sel4platsupport/timer.h>

#include <vka/vka.h>
#include <vka/object.h>
#include <vspace/vspace.h>

#define DEFAULT_TIMER_INTERRUPT MSI_MIN

/**
 * Initialise the hpet timer by parsing the acpi tables and do all seL4 specific set up.
 *
 * @param acpi initialised acpi structure (call sel4platsupport_acpi_init)
 * @param notification notification object for the irq to come in on
 * @param irq_number irq number in the MSI range that HPET irqs should come in on.
 * @return interface with hpet_data_t above. Users can use notification to wait for irqs,
 * or bind it to other endpoints.
 */
seL4_timer_t *sel4platsupport_get_hpet(vspace_t *vspace, simple_t *simple, acpi_t *acpi,
                                       vka_t *vka, seL4_CPtr notification, uint32_t irq_number);


/**
 * Initialise the hpet timer with provided frame and do all seL4 specific set up.
 *
 * @param vka allocator to allocate resources and frame at paddr
 * @param paddr physical location of the hpet
 * @param notification notification object for the irq to come in on
 * @param irq_number irq number in the MSI range that HPET irqs should come in on.
 * @return interface with hpet_data_t above. Users can use notification to wait for irqs,
 * or bind it to other endpoints.
 */
seL4_timer_t * sel4platsupport_get_hpet_paddr(vspace_t *vspace, simple_t *simple, vka_t *vka,
                                            uintptr_t paddr, seL4_CPtr notification, uint32_t irq_number);
#endif /* __SEL4_PLATSUPPORT_HPET_H */

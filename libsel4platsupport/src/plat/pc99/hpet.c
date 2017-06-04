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
#include <sel4/sel4.h>
#include <platsupport/plat/hpet.h>
#include <sel4platsupport/plat/hpet.h>
#include <sel4platsupport/plat/pit.h>
#include <sel4platsupport/device.h>
#include <sel4platsupport/io.h>
#include <utils/util.h>
#include <stdint.h>
#include <string.h>
#include <utils/attribute.h>
#include <vka/capops.h>
#include "../../timer_common.h"

static void
hpet_handle_irq(seL4_timer_t *timer, uint32_t irq)
{
    timer_handle_irq(timer->timer, irq);
    seL4_IRQHandler_Ack(timer->irq);
}

static void
hpet_destroy(seL4_timer_t *timer, vka_t *vka, vspace_t *vspace)
{
    timer_common_destroy_frame(timer, vka, vspace);
    /* clear the irq */
    seL4_IRQHandler_Clear(timer->irq);
}

static uintptr_t
hpet_get_addr(acpi_t *acpi)
{
    acpi_header_t *header = acpi_find_region(acpi, ACPI_HPET);
    if (header == NULL) {
        ZF_LOGE("Failed to find HPET acpi table\n");
        return 0;
    }
    /* find the physical address of the timer */
    acpi_hpet_t *hpet_header = (acpi_hpet_t *) header;
    return (uintptr_t) hpet_header->base_address.address;
}

static acpi_t *
hpet_init_acpi(vka_t *vka, vspace_t *vspace)
{
    ps_io_mapper_t mapper;
    int error = sel4platsupport_new_io_mapper(*vspace, *vka, &mapper);
    if (error) {
        ZF_LOGE("Failed to get io mapper");
        return NULL;
    }

    acpi_t *acpi = acpi_init(mapper);
    if (acpi == NULL) {
        ZF_LOGE("Failed to init acpi tables");
    }

    return acpi;
}

uintptr_t
sel4platsupport_get_default_timer_paddr(vka_t *vka, vspace_t *vspace)
{
    acpi_t *acpi = hpet_init_acpi(vka, vspace);

    if (acpi != NULL) {
        return hpet_get_addr(acpi);
    }
    return 0;
}

seL4_timer_t *
sel4platsupport_get_hpet_paddr(vspace_t *vspace, simple_t *simple, vka_t *vka,
                               uintptr_t paddr, seL4_CPtr notification, int irq_number, int vector)
{
    seL4_timer_t *hpet = calloc(1, sizeof(seL4_timer_t));
    if (hpet == NULL) {
        ZF_LOGE("Failed to allocate hpet_t of size %zu\n", sizeof(seL4_timer_t));
        return NULL;
    }

    if (timer_common_init_frame(hpet, vspace, vka, paddr) != 0) {
        free(hpet);
        return NULL;
    }

    hpet->destroy = hpet_destroy;

    /* initialize the IRQ */
    cspacepath_t path;
    int error;
    int ioapic;
    if (irq_number == -1) {
        error = sel4platsupport_copy_msi_cap(vka, simple, vector, &path);
        /* set irq_number to vector for passing to hpet_get_timer. As the kernel
         * does not currently perform interrupt routing we need to manually
         * add the IRQ_OFFSET to the deliver to construct the correct
         * finaly vector */
        irq_number = vector + IRQ_OFFSET;
        ioapic = 0;
    } else {
        error = sel4platsupport_copy_ioapic_cap(vka, simple, 0, irq_number, 0, 1, vector, &path);
        ioapic = 1;
    }
    hpet->handle_irq = hpet_handle_irq;
    hpet->irq = path.capPtr;
    if (error != seL4_NoError) {
        ZF_LOGE("Failed to get IRQ cap, error %d", error);
        timer_common_destroy(hpet, vka, vspace);
        return NULL;
    }

    error = seL4_IRQHandler_SetNotification(path.capPtr, notification);
    if (error != seL4_NoError) {
        ZF_LOGE("seL4_IRQHandler_SetNotification failed with error %d", error);
        timer_common_destroy(hpet, vka, vspace);
        return NULL;
    }

    /* finall initialise the timer */
    hpet_config_t config = {
        .vaddr = hpet->vaddr,
        .irq = irq_number,
        .ioapic_delivery = ioapic,
    };

    hpet->timer = hpet_get_timer(&config);
    if (hpet->timer == NULL) {
        timer_common_destroy(hpet, vka, vspace);
        return NULL;
    }

    /* success */
    return hpet;
}

seL4_timer_t *
sel4platsupport_get_hpet(vspace_t *vspace, simple_t *simple, acpi_t *acpi,
                         vka_t *vka, seL4_CPtr notification, int irq_number, int vector)
{

    uintptr_t addr = hpet_get_addr(acpi);
    if (addr == 0) {
        ZF_LOGE("Failed to find hpet");
        return NULL;
    }

    return sel4platsupport_get_hpet_paddr(vspace, simple, vka, addr, notification, irq_number, vector);
}

seL4_timer_t *
sel4platsupport_get_default_timer(vka_t *vka, vspace_t *vspace,
                                  simple_t *simple, seL4_CPtr notification)
{
    seL4_timer_t *timer = NULL;

    acpi_t *acpi = hpet_init_acpi(vka, vspace);
    if (acpi != NULL) {
        /* First try the HPET -> this will only work if the calling task can parse
         * acpi */
        /* We have to make up a vector number to use, this is incredibly unsafe, but this
         * interface gives us no choice */
        timer = sel4platsupport_get_hpet(vspace, simple, acpi, vka, notification, -1, 37);
        if (!timer) {
            ZF_LOGW("Arbitrarily allocated vector 37, hopefully it does not collide");
        }
    }

    if (timer == NULL) {
        /* fall back to the the PIT */
        timer = sel4platsupport_get_pit(vka, simple, NULL, notification);
    }

    if (timer == NULL) {
        ZF_LOGE("Failed to init a default timer!");
    }

    return timer;
}

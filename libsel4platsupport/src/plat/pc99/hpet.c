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
#include <sel4/sel4.h>
#include <platsupport/plat/hpet.h>
#include <sel4platsupport/plat/hpet.h>
#include <sel4platsupport/device.h>
#include <sel4platsupport/io.h>
#include <utils/util.h>
#include <stdint.h>
#include <string.h>
#include <utils/attribute.h>
#include <vka/capops.h>
#include "../../timer_common.h"

#ifdef CONFIG_LIB_SEL4_VSPACE

static void
hpet_handle_irq_msi(seL4_timer_t *timer, uint32_t irq)
{
    timer_common_data_t *data = (timer_common_data_t *) timer->data;
    timer_handle_irq(timer->timer, irq + IRQ_OFFSET);
    seL4_IRQHandler_Ack(data->irq);
}

static void UNUSED
hpet_handle_irq_ioapic(seL4_timer_t *timer, uint32_t irq)
{
    timer_common_data_t *data = (timer_common_data_t *) timer->data;
    timer_handle_irq(timer->timer, irq);
    seL4_IRQHandler_Ack(data->irq);
}

static void
hpet_destroy(seL4_timer_t *timer, vka_t *vka, vspace_t *vspace)
{
    timer_common_data_t *timer_data = (timer_common_data_t *) timer->data;
    timer_common_destroy_frame(timer, vka, vspace);
    /* clear the irq */
    seL4_IRQHandler_Clear(timer_data->irq);
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
                               uintptr_t paddr, seL4_CPtr notification, uint32_t irq_number)
{
    seL4_timer_t *hpet = NULL;
    int ioapic = 0;
    int irq = -1;

    hpet = (seL4_timer_t *)calloc(1, sizeof(seL4_timer_t));
    if (hpet == NULL) {
        ZF_LOGE("Failed to allocate hpet_t of size %zu\n", sizeof(seL4_timer_t));
        goto error;
    }

    timer_common_data_t *hpet_data = timer_common_init_frame(vspace, vka, paddr);
    if (hpet_data == NULL) {
        goto error;
    }

    /* check what range the IRQ is in */
    hpet->destroy = hpet_destroy;

    if ((int)irq_number >= MSI_MIN && irq_number <= MSI_MAX) {
        irq = irq_number + IRQ_OFFSET;
        ioapic = 0;
        hpet->handle_irq = hpet_handle_irq_msi;
    }
    if (irq == -1) {
        ZF_LOGE("IRQ %u is not valid\n", irq_number);
        goto error;
    }
    /* initialise msi irq */
    cspacepath_t path;
    int error = sel4platsupport_copy_msi_cap(vka, simple, irq, &path);
    hpet_data->irq = path.capPtr;
    if (error != seL4_NoError) {
        ZF_LOGE("Failed to get msi cap, error %d\n", error);
        goto error;
    }

    error = seL4_IRQHandler_SetNotification(path.capPtr, notification);
    if (error != seL4_NoError) {
        ZF_LOGE("seL4_IRQHandler_SetNotification failed with error %d\n", error);
        goto error;
    }

    hpet->data = (void *) hpet_data;

    /* finall initialise the timer */
    hpet_config_t config = {
        .vaddr = hpet_data->vaddr,
        .irq = irq,
        .ioapic_delivery = ioapic,
    };

    hpet->timer = hpet_get_timer(&config);
    if (hpet->timer == NULL) {
        goto error;
    }

    /* success */
    return hpet;

error:
    timer_common_destroy(hpet, vka, vspace);
    return NULL;
}

seL4_timer_t *
sel4platsupport_get_hpet(vspace_t *vspace, simple_t *simple, acpi_t *acpi,
                         vka_t *vka, seL4_CPtr notification, uint32_t irq_number)
{

    uintptr_t addr = hpet_get_addr(acpi);
    if (addr == 0) {
        ZF_LOGE("Failed to find hpet");
        return NULL;
    }

    return sel4platsupport_get_hpet_paddr(vspace, simple, vka, addr, notification, irq_number);
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
        timer = sel4platsupport_get_hpet(vspace, simple, acpi, vka, notification, MSI_MIN);
    }

    if (timer == NULL) {
        /* fall back to the the PIT */
        ps_io_ops_t io_ops;
        if (sel4platsupport_new_io_ops(*vspace, *vka, &io_ops) == 0) {
            timer = sel4platsupport_get_pit(vka, simple, &io_ops.io_port_ops, notification);
        }
    }

    if (timer == NULL) {
        ZF_LOGE("Failed to init a default timer!");
    }

    return timer;
}

#endif /* CONFIG_LIB_SEL4_VSPACE */

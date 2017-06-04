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

#include <simple/simple.h>
#include <sel4platsupport/plat/timer.h>
#include <sel4platsupport/plat/serial.h>
#include <sel4platsupport/device.h>
#include <platsupport/plat/hpet.h>
#include <vka/capops.h>

int
sel4platsupport_arch_init_default_timer_caps(vka_t *vka, vspace_t *vspace, simple_t *simple, timer_objects_t *timer_objects)
{
    int error;

    /* map in the timer paddr so that we can query the HPET properties. Although
     * we only support the possiblity of non FSB delivery if we are using the IOAPIC */
    cspacepath_t frame;
    error = vka_cspace_alloc_path(vka, &frame);
    if (error) {
        ZF_LOGF("Failed to allocate cslot");
        return error;
    }
    error = vka_untyped_retype(&timer_objects->timer_dev_ut_obj, seL4_X86_4K, seL4_PageBits, 1, &frame);
    if (error) {
        ZF_LOGF("Failed to retype timer frame");
        return error;
    }
    void *vaddr;
    vaddr = vspace_map_pages(vspace, &frame.capPtr, NULL, seL4_AllRights, 1, seL4_PageBits, 0);
    if (vaddr == NULL) {
        ZF_LOGF("Failed to map HPET paddr");
        return -1;
    }
    if (!hpet_supports_fsb_delivery(vaddr)) {
        if (!config_set(CONFIG_IRQ_IOAPIC)) {
            ZF_LOGF("HPET does not support FSB delivery and we are not using the IOAPIC");
        }
        uint32_t irq_mask = hpet_ioapic_irq_delivery_mask(vaddr);
        /* grab the first irq */
        int irq = FFS(irq_mask) - 1;
        error = arch_simple_get_ioapic(&simple->arch_simple, timer_objects->timer_irq_path, 0, irq, 0, 1, DEFAULT_TIMER_INTERRUPT);
    } else {
        error = arch_simple_get_msi(&simple->arch_simple, timer_objects->timer_irq_path, 0, 0, 0, 0, DEFAULT_TIMER_INTERRUPT);
    }
    if (error) {
        ZF_LOGF("Failed to obtain IRQ cap for PS default timer.");
        return error;
    }
    vspace_unmap_pages(vspace, vaddr, 1, seL4_PageBits, VSPACE_PRESERVE);
    vka_cnode_delete(&frame);
    vka_cspace_free(vka, frame.capPtr);
    return 0;
}

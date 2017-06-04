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

#include <vka/vka.h>
#include <vspace/vspace.h>
#include <simple/simple.h>
#include <sel4platsupport/timer.h>
#include <sel4platsupport/device.h>

int sel4platsupport_plat_init_default_timer_caps(vka_t *vka, UNUSED vspace_t *vspace, simple_t *simple, timer_objects_t *timer_objects)
{
    /* Platforms that use a different device/driver to satisfy their wall-clock
     * timer requirement may be initialized here.
     */
    plat_timer_objects_t *plat_timer_objects = &timer_objects->arch_timer_objects.plat_timer_objects;

    int error = sel4platsupport_copy_irq_cap(vka, simple, GPT2_INTERRUPT, &plat_timer_objects->clock_irq_path);
    if (error != 0) {
        ZF_LOGF("Failed to get GPT2_INTERRUPT");
        return error;
    }

    error = vka_alloc_untyped_at(vka, seL4_PageBits, GPT2_DEVICE_PADDR, &plat_timer_objects->clock_timer_dev_ut_obj);
    if (error != 0) {
        ZF_LOGF("Failed to allocate GPT2 device-untyped");
        return error;
    }
    return 0;
}

seL4_timer_t *sel4platsupport_get_default_timer(vka_t *vka, vspace_t *vspace, simple_t *simple, seL4_CPtr notification)
{
    return sel4platsupport_get_rel_gpt(vspace, simple, vka, notification, GPT1, 1);
}

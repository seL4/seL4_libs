/*
* Copyright 2017, Data61
* Commonwealth Scientific and Industrial Research Organisation (CSIRO)
* ABN 41 687 119 230.
*
* This software may be distributed and modified according to the terms of
* the BSD 2-Clause license. Note that NO WARRANTY is provided.
* See "LICENSE_BSD2.txt" for details.
*
* @TAG(D61_BSD)
*/

#include <autoconf.h>
#include <sel4platsupport/timer.h>




int sel4platsupport_init_default_timer_caps(vka_t *vka, vspace_t *vspace, simple_t *simple, timer_objects_t *timer_objects)
{
    int error;

    /* Allocate slot for the timer IRQ. */
    error = vka_cspace_alloc_path(vka, &timer_objects->timer_irq_path);
    if (error) {
        ZF_LOGF("Failed to allocate timer IRQ slot.");
        return error;
    }

    /* Obtain frame cap for PS default timer.
    * Note: We keep the timer's MMIO physical address as an untyped, for the
    * timer, but for the serial we retype it immediately as a frame.
    *
    * The reason for this is that we would prefer to pass untypeds, but since
    * the test driver and test child both use the serial-frame, we can't share
    * it between them as an untyped, so we must retype it as a frame first, so
    * that the cap can be cloned.
    */
    timer_objects->timer_paddr = sel4platsupport_get_default_timer_paddr(vka, vspace);
    error = vka_alloc_untyped_at(vka, seL4_PageBits, timer_objects->timer_paddr,
        &timer_objects->timer_dev_ut_obj);
    if (error) {
        ZF_LOGF("Failed to obtain device-ut cap for default timer.");
        return error;
    }
    /* Then call into the arch- and plat-specific code to init all arch-
    * and plat-specific code. Some platforms need another timer because they
    * use different timers/drivers for the event-timer and the
    * wall-clock-timer.
    */
    return sel4platsupport_arch_init_default_timer_caps(vka, vspace, simple, timer_objects);
}

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
#include <vka/vka.h>
#include <vspace/vspace.h>

#include <sel4platsupport/timer.h>
#include <platsupport/arch/generic_timer.h>

#include <stdlib.h>

#include <simple/simple.h>
#include <vka/capops.h>
#include <sel4platsupport/device.h>

#ifdef CONFIG_ARCH_ARM_V7A
#ifdef CONFIG_ARM_CORTEX_A15
#ifdef CONFIG_EXPORT_PCNT_USER

seL4_timer_t *
sel4platsupport_get_generic_timer(void)
{

    seL4_timer_t *timer = calloc(1, sizeof(seL4_timer_t));
    if (timer == NULL) {
        return NULL;
    }

    timer->timer = generic_timer_get_timer();
    return timer;
}

#endif /* CONFIG_EXPORT_PCNT_USER */
#endif /* CONFIG_ARM_CORTEX_A15 */
#endif /* CONFIG_ARCH_ARM_V7A */

uintptr_t
sel4platsupport_get_default_timer_paddr(UNUSED vka_t *vka, UNUSED vspace_t *vspace)
{
    return DEFAULT_TIMER_PADDR;
}

int
sel4platsupport_arch_init_default_timer_caps(vka_t *vka, vspace_t *vspace, simple_t *simple, timer_objects_t *timer_objects)
{
    int error;

    /* Obtain the IRQ cap for the PS default timer IRQ
     * The slot was allocated earlier, outside in init_timer_caps.
     *
     * The IRQ cap setup is arch specific because x86 uses MSI, and that's
     * a different function.
     */
    error = sel4platsupport_copy_irq_cap(vka, simple, DEFAULT_TIMER_INTERRUPT,
                                             &timer_objects->timer_irq_path);
    if (error) {
        ZF_LOGF("Failed to obtain PS default timer IRQ cap.");
        return error;
    }

    return sel4platsupport_plat_init_default_timer_caps(vka, vspace, simple, timer_objects);
}

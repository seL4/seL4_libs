/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _SEL4_PLATSUPPORT_TIMER_H
#define _SEL4_PLATSUPPORT_TIMER_H

#include <sel4platsupport/timer_types.h>
#include <sel4platsupport/plat/timer.h>
#include <sel4platsupport/arch/timer.h>

#include <platsupport/timer.h>
#include <simple/simple.h>
#include <vka/vka.h>
#include <vka/object.h>
#include <vspace/vspace.h>

static inline void
sel4_timer_handle_irq(seL4_timer_t *timer, uint32_t irq)
{
    timer->handle_irq(timer, irq);
}

static inline void
sel4_timer_destroy(seL4_timer_t *timer, vka_t *vka, vspace_t *vspace)
{
    if (timer->destroy == NULL) {
        ZF_LOGF("This timer doesn't support destroy!");
    }

    timer->destroy(timer, vka, vspace);
}

/* This is a helper function that assumes a timer has a single IRQ
 * finds it and handles it */
static inline void
sel4_timer_handle_single_irq(seL4_timer_t *timer)
{
    assert(timer != NULL);
    assert(timer->timer->properties.irqs == 1);
    sel4_timer_handle_irq(timer, timer_get_nth_irq(timer->timer, 0));
}

/*
 * Get the default timer for this platform.
 *
 * The default timer, at minimum, supports setting periodic timeouts for small intervals.
 *
 * @param vka an initialised vka implementation that can allocate the physical
 *        address returned by sel4platsupport_get_default_timer_paddr.
 * @param notification endpoint capability for irqs to be delivered to.
 * @return initialised timer.
 */
seL4_timer_t * sel4platsupport_get_default_timer(vka_t *vka, vspace_t *vspace, simple_t *simple,
                                                 seL4_CPtr notification);

/*
 * @return the physical address for the default timer.
 */
uintptr_t sel4platsupport_get_default_timer_paddr(vka_t *vka, vspace_t *vspace);
#endif /* _SEL4_PLATSUPPORT_TIMER_H */

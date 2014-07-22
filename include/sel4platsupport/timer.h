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

#include <platsupport/timer.h>

#include <simple/simple.h>

#include <vka/vka.h>
#include <vka/object.h>
#include <vspace/vspace.h>

typedef struct seL4_timer seL4_timer_t;

typedef void (*handle_irq_fn_t)(seL4_timer_t *timer, uint32_t irq);

struct seL4_timer {
    /* os independant timer interface */
    pstimer_t *timer;

    /* timer specific config data. View in platsupport/plat/<timer>.h */
    void *data;

    /* sel4 specific functions to call to deal with timer */
    handle_irq_fn_t handle_irq;
};

static inline void
sel4_timer_handle_irq(seL4_timer_t *timer, uint32_t irq)
{
    timer->handle_irq(timer, irq);
}

/* some timers use this as their config data. Check the timer specific header file to see */
typedef struct {
    /* frame cap to the physical frame the timer is mapped in to */
    cspacepath_t frame;
    /* irq cap */
    seL4_CPtr irq;
    /* vaddr that frame is mapped in at */
    void *vaddr;
} timer_common_data_t;

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
 * @param aep endpoint capability for irqs to be delivered to. 
 * @return initialised timer.
 */
seL4_timer_t * sel4platsupport_get_default_timer(vka_t *vka, vspace_t *vspace, simple_t *simple,
        seL4_CPtr aep);

#endif /* _SEL4_PLATSUPPORT_TIMER_H */

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _SEL4PLATSUPPORT_TIMER_INTERNAL_H
#define _SEL4PLATSUPPORT_TIMER_INTERNAL_H

#include <vspace/vspace.h>

#include <vka/vka.h>

#include <simple/simple.h>

#include <sel4platsupport/timer.h>


/*
 * Allocate a cslot for an irq and get the cap for that irq.
 *
 * @param irq_number to get the cap to
 * @return the cap to the irq
 */
seL4_CPtr timer_common_get_irq(vka_t *vka, simple_t *simple, uint32_t irq_number);

void timer_common_cleanup_irq(vka_t *vka, seL4_CPtr irq);

void timer_common_handle_irq(seL4_timer_t *timer, uint32_t irq);
/**
 *
 * Some of our timers require 1 irq, 1 frame and an aep.
 *
 * If this is true, timers can do most of their sel4 based init with this function.
 *
 * @param irq_number the irq that the timer needs a cap for.
 * @param paddr the paddr that the timer needs mapped in.
 * @param aep the badged async endpoint that the irq should be delivered to.
 */
timer_common_data_t *timer_common_init(vspace_t *vspace, simple_t *simple,
                                       vka_t *vka, seL4_CPtr aep, uint32_t irq_number, void *paddr);

/*
 * Something went wrong, clean up everything we did in timer_common_init.
 */
void timer_common_destroy(timer_common_data_t *data, vka_t *vka, vspace_t *vspace);

#endif /* _SEL4PLATSUPPORT_TIMER_INTERNAL_H */

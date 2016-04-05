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

/**
 *
 * Some of our timers require 1 irq, 1 frame and an notification.
 *
 * If this is true, timers can do most of their sel4 based init with this function.
 *
 * @param irq_number the irq that the timer needs a cap for.
 * @param paddr the paddr that the timer needs mapped in.
 * @param notification the badged notification object that the irq should be delivered to.
 */
timer_common_data_t *timer_common_init(vspace_t *vspace, simple_t *simple,
                                       vka_t *vka, seL4_CPtr notification, uint32_t irq_number, void *paddr);

/*
 * Something went wrong, clean up everything we did in timer_common_init.
 */
void timer_common_destroy(seL4_timer_t *timer, vka_t *vka, vspace_t *vspace);
void timer_common_cleanup_irq(vka_t *vka, seL4_CPtr irq);

void timer_common_handle_irq(seL4_timer_t *timer, uint32_t irq);

#endif /* _SEL4PLATSUPPORT_TIMER_INTERNAL_H */

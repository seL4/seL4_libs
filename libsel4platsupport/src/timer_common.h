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
seL4_timer_t *timer_common_init(vspace_t *vspace, simple_t *simple,
                                       vka_t *vka, seL4_CPtr notification, uint32_t irq_number, void *paddr);

/*
 * clean up functions
 */
void timer_common_destroy_frame(seL4_timer_t *timer, vka_t *vka, vspace_t *vspace);
void timer_common_destroy(seL4_timer_t *timer, vka_t *vka, vspace_t *vspace);
void timer_common_cleanup_irq(vka_t *vka, seL4_CPtr irq);

void timer_common_handle_irq(seL4_timer_t *timer, uint32_t irq);
int timer_common_init_frame(seL4_timer_t *timer, vspace_t *vspace, vka_t *vka, uintptr_t paddr);

#endif /* _SEL4PLATSUPPORT_TIMER_INTERNAL_H */

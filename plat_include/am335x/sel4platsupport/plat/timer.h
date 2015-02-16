/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef _SEL4PLATSUPPORT_PLAT_TIMER_H
#define _SEL4PLATSUPPORT_PLAT_TIMER_H

#include <sel4platsupport/timer.h>
#include <platsupport/plat/timer.h>

#define DEFAULT_TIMER_PADDR DMTIMER2_PADDR
#define DEFAULT_TIMER_INTERRUPT DMTIMER2_INTERRUPT

seL4_timer_t *sel4platsupport_get_timer(enum timer_id id,
                                        vka_t *vka,
                                        vspace_t *vspace,
                                        simple_t *simple,
                                        seL4_CPtr aep);

#endif /* _SEL4PLATSUPPORT_PLAT_TIMER_H */

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
#ifndef _SEL4PLATSUPPORT_PLAT_TIMER_H
#define _SEL4PLATSUPPORT_PLAT_TIMER_H

#include <sel4platsupport/timer_types.h>

#include <platsupport/plat/timer.h>

#define DEFAULT_TIMER_PADDR KPSS_TIMERS_PADDR
#define DEFAULT_TIMER_INTERRUPT KPSS_GPT0_INTERRUPT

typedef struct plat_timer_objects {
} plat_timer_objects_t;


seL4_timer_t *sel4platsupport_get_timer(enum timer_id id,
                                        vka_t *vka,
                                        vspace_t *vspace,
                                        simple_t *simple,
                                        seL4_CPtr notification);

#include <sel4platsupport/arch/timer.h>
#include <sel4platsupport/timer.h>

#endif /* _SEL4PLATSUPPORT_PLAT_TIMER_H */

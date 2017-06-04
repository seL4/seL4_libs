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

#include <platsupport/arch/tsc.h>
#include <sel4platsupport/plat/hpet.h>

/**
 * Get a tsc backed seL4_timer_t;
 *
 * @param freq frequency of the tsc.
 * @return NULL on error.
 */
seL4_timer_t *sel4platsupport_get_tsc_timer_freq(uint64_t freq);

/**
 * Get a tsc backed seL4_timer_t;
 *
 * Note that measuring the tsc frequency is expensive, so use of this function should be minimal.
 *
 * @param timeout_timer timer that implements timeouts to calculate the CPU frequency with.
 * @return NULL on error.
 */

seL4_timer_t *sel4platsupport_get_tsc_timer(seL4_timer_t *timeout_timer);

#include <sel4platsupport/arch/timer.h>
#include <sel4platsupport/timer.h>

#endif /* _SEL4PLATSUPPORT_PLAT_TIMER_H */

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

#include <platsupport/arch/tsc.h>
#include <sel4platsupport/plat/hpet.h>
#include <sel4platsupport/plat/pit.h>

/**
 * Get a tsc backed seL4_timer_t;
 *
 * @param timeout_timer timer that implements timeouts to calculate the CPU frequency with.
 * @return NULL on error.
 */
seL4_timer_t *sel4platsupport_get_tsc_timer(seL4_timer_t *timeout_timer);

#endif /* _SEL4PLATSUPPORT_PLAT_TIMER_H */

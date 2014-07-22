/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/*
 * The gpt is an upcounter.
 */
#ifndef __SEL4_PLAT_SUPPORT_GPT_H
#define __SEL4_PLAT_SUPPORT_GPT_H

#include <autoconf.h>

#include <stdint.h>
#include <sel4platsupport/timer.h>


/**
 * Get an interface for an initialised gpt timer.
 *
 * @param aep async endpoint for the irq to come in on
 * @param prescaler to scale time by. 0 = divide by 1. 1 = divide by 2, ...
 */
seL4_timer_t *
sel4platsupport_get_gpt(vspace_t *vspace, simple_t *simple, vka_t *vka, seL4_CPtr aep, 
        uint32_t prescaler);
void sel4platsupport_destroy_gpt(seL4_timer_t *timer, vka_t *vka, vspace_t *vspace);

#endif /* __SEL4_PLAT_SUPPORT_EPIT_H */

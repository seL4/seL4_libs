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
#include <platsupport/plat/gpt_constants.h>

#define CLOCK_TIMER_PADDR GPT1_DEVICE_PADDR
#define CLOCK_TIMER_INTERRUPT GPT1_INTERRUPT

/**
 * Get an interface for an initialised gpt timer.
 *
 * @param notification notification object for the irq to come in on
 * @param prescaler to scale time by. 0 = divide by 1. 1 = divide by 2, ...
 */
seL4_timer_t *
sel4platsupport_get_gpt(vspace_t *vspace, simple_t *simple, vka_t *vka, seL4_CPtr notification,
                        uint32_t prescaler);
#endif /* __SEL4_PLAT_SUPPORT_EPIT_H */

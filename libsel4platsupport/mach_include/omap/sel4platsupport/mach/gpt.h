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
 * Note that you cannot use periodic and oneshot timeouts simultaneously.
 */
#ifndef __SEL4_PLAT_SUPPORT_GPT_H
#define __SEL4_PLAT_SUPPORT_GPT_H

#include <autoconf.h>

#include <stdint.h>

#include <sel4platsupport/timer.h>
#include <platsupport/timer.h>
#include <platsupport/plat/timer.h>

#include <vspace/vspace.h>
#include <vka/vka.h>
#include <simple/simple.h>

#define DEFAULT_TIMER_PADDR GPT1_DEVICE_PADDR
#define DEFAULT_TIMER_INTERRUPT GPT1_INTERRUPT

typedef timer_common_data_t gpt_data;

/**
 * Get an interface for an initialised gpt timer.
 *
 * @deprecated use sel4platsupport_get_rel_gpt
 * @param notification notification object for the irq to come in on
 * @param prescaler to scale time by. 0 = divide by 1. 1 = divide by 2, ...
 * @param gpt_id some platforms have more than one gpt, pick one.
 */
DEPRECATED("use sel4platsupport_get_rel_gpt")
seL4_timer_t *sel4platsupport_get_gpt(vspace_t *vspace, simple_t *simple, vka_t *vka,
                                      seL4_CPtr notification, gpt_id_t id, uint32_t prescaler);
/** 
 * Get a GPT timer driver that supports relative and periodic timeouts.
 *
 * @param notification notification object for the irq to come in on
 * @param prescaler to scale time by. 0 = divide by 1. 1 = divide by 2, ...
 * @param gpt_id some platforms have more than one gpt, pick one.
 */
seL4_timer_t * 
sel4platsupport_get_rel_gpt(vspace_t *vspace, simple_t *simple, vka_t *vka, seL4_CPtr notification,
                            gpt_id_t gpt_id, uint32_t prescaler);
/** 
 * Get a GPT timer driver that absolute and relative one shot timeouts, and 
 * tracks a 64bit current time.
 *
 * @param notification notification object for the irq to come in on
 * @param prescaler to scale time by. 0 = divide by 1. 1 = divide by 2, ...
 * @param gpt_id some platforms have more than one gpt, pick one.
 */
seL4_timer_t *
sel4platsupport_get_abs_gpt(vspace_t *vspace, simple_t *simple, vka_t *vka, seL4_CPtr notification, 
                            gpt_id_t gpt_id, uint32_t prescaler);
              
#endif /* __SEL4_PLAT_SUPPORT_GPT_H */

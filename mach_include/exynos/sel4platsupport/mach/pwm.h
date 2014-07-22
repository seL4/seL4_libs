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
#ifndef __SEL4_PLAT_SUPPORT_PWM_H
#define __SEL4_PLAT_SUPPORT_PWM_H

#include <sel4platsupport/timer.h>
#include <platsupport/timer.h>
#include <platsupport/plat/timer.h>

#include <vspace/vspace.h>
#include <vka/vka.h>
#include <simple/simple.h>

typedef timer_common_data_t pwm_data;


#define DEFAULT_TIMER_PADDR PWM_TIMER_PADDR
#define DEFAULT_TIMER_INTERRUPT PWM_T4_INTERRUPT

/**
 * Get an interface for an initialised pwm timer.
 *
 * @param aep async endpoint for the irq to come in on
 */
seL4_timer_t *sel4platsupport_get_pwm(vspace_t *vspace, simple_t *simple, vka_t *vka,
        seL4_CPtr aep);

void sel4platsupport_destroy_pwm(seL4_timer_t *timer, vka_t *vka, vspace_t *vspace);

#endif /* __SEL4_PLAT_SUPPORT_PWM_H */

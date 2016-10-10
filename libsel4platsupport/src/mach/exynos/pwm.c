/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sel4platsupport/timer.h>
#include <sel4platsupport/plat/timer.h>
#include <platsupport/mach/pwm.h>
#include <utils/util.h>
#include "../../timer_common.h"

seL4_timer_t *
sel4platsupport_get_pwm(vspace_t *vspace, simple_t *simple, vka_t *vka, seL4_CPtr notification)
{

    seL4_timer_t *timer = timer_common_init(vspace, simple, vka, notification,
                                            PWM_T4_INTERRUPT, (void *) PWM_TIMER_PADDR);

    if (timer == NULL) {
        timer_common_destroy(timer, vka, vspace);
        return NULL;
    }

    /* do hardware init */
    pwm_config_t config = {
        .vaddr = timer->vaddr,
    };

    timer->timer = pwm_get_timer(&config);
    if (timer->timer == NULL) {
        timer_common_destroy(timer, vka, vspace);
        return NULL;
    }

    return timer;
}

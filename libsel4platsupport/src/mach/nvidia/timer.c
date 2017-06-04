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

#include <autoconf.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vka/vka.h>
#include <vspace/vspace.h>
#include <sel4platsupport/timer.h>
#include <sel4platsupport/plat/timer.h>
#include <platsupport/plat/timer.h>
#include <utils/util.h>
#include "../../timer_common.h"

int sel4platsupport_plat_init_default_timer_caps(UNUSED vka_t *vka, UNUSED vspace_t *vspace, UNUSED simple_t *simple, UNUSED timer_objects_t *timer_objects)
{
    return 0;
}

seL4_timer_t *
sel4platsupport_get_default_timer(vka_t *vka, vspace_t *vspace, simple_t *simple, seL4_CPtr notification)
{
    seL4_timer_t *timer = timer_common_init(vspace, simple, vka, notification,
                                            DEFAULT_TIMER_INTERRUPT, (void *) DEFAULT_TIMER_PADDR);

    if (timer == NULL) {
        return NULL;
    }

    nv_tmr_config_t config = {
        .vaddr = timer->vaddr + TMR1_OFFSET,
        .tmrus_vaddr = timer->vaddr + TMRUS_OFFSET,
        .shared_vaddr = timer->vaddr + TMR_SHARED_OFFSET,
        .irq = DEFAULT_TIMER_INTERRUPT
    };
    timer->timer = nv_get_timer(&config);
    if (timer->timer == NULL) {
        timer_common_destroy(timer, vka, vspace);
    }
    return timer;
}


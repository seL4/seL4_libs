/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
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

seL4_timer_t *
sel4platsupport_get_default_timer(vka_t *vka, vspace_t *vspace, simple_t *simple, seL4_CPtr notification)
{
    void *paddr = (void *)DEFAULT_TIMER_PADDR;
    uint32_t irq = DEFAULT_TIMER_INTERRUPT;
    sel4_timer_t *timer = timer_common_init(vspace, simple, vka, notification, irq, paddr);

    if (timer->data == NULL) {
        return NULL;
    }

    nv_tmr_config_t config = {
        .vaddr = timer->vaddr + TMR1_OFFSET,
        .tmrus_vaddr = timer->vaddr + TMRUS_OFFSET,
        .shared_vaddr = timer->vaddr + TMR_SHARED_OFFSET,
        .irq = irq
    };
    timer->timer = tk1_get_timer(&config);
    if (timer->timer == NULL) {
        timer_common_destroy(timer, vka, vspace);
    }
    return timer;
}


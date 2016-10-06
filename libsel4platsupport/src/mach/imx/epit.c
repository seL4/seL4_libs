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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sel4platsupport/timer.h>
#include <sel4platsupport/mach/epit.h>
#include <platsupport/mach/epit.h>
#include <utils/util.h>
#include "../../timer_common.h"


seL4_timer_t *
sel4platsupport_get_epit(vspace_t *vspace, simple_t *simple, vka_t *vka, seL4_CPtr notification,
                         uint32_t prescaler, epit_id_t epit_id)
{

#ifdef CONFIG_PLAT_IMX31
    if (epit_id == EPIT1) {
        ZF_LOGE("Epit1 is taken by the kernel\n");
        return NULL;
    }
#endif

    /* check the id */
    if (epit_id != EPIT1 && epit_id != EPIT2) {
        ZF_LOGE("Invalid epit id %d\n", epit_id);
        return NULL;
    }

    /* find paddr/irq */
    void *paddr = (void *) (epit_id == EPIT1 ? EPIT1_DEVICE_PADDR : EPIT2_DEVICE_PADDR);
    uint32_t irq = (epit_id == EPIT1 ? EPIT1_INTERRUPT : EPIT2_INTERRUPT);

    seL4_timer_t *timer = timer_common_init(vspace, simple, vka, notification, irq, paddr);
    if (timer == NULL) {
        return NULL;
    }

    /* do hardware init */
    epit_config_t config = {
        .vaddr = timer->vaddr,
        .irq = irq,
        .prescaler = prescaler
    };

    timer->timer = epit_get_timer(&config);
    if (timer->timer == NULL) {
        timer_common_destroy(timer, vka, vspace);
        return NULL;
    }

    /* success */
    return timer;
}


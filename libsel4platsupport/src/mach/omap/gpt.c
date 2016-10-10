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
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sel4platsupport/timer.h>
#include <sel4platsupport/plat/timer.h>
#include <platsupport/mach/gpt.h>
#include <utils/util.h>
#include "../../timer_common.h"


static seL4_timer_t *
sel4platsupport_get_gpt_impl(vspace_t *vspace, simple_t *simple, vka_t *vka, seL4_CPtr notification,
                        gpt_id_t gpt_id, uint32_t prescaler, bool relative)
{

    /* check the id */
    if (gpt_id > GPT_LAST) {
        ZF_LOGE("Incorrect GPT id %d\n", gpt_id);
        return NULL;
    }

    /* find paddr/irq */
    void *paddr = omap_get_gpt_paddr(gpt_id);
    uint32_t irq = omap_get_gpt_irq(gpt_id);

    seL4_timer_t *timer = timer_common_init(vspace, simple, vka, notification, irq, paddr);
    if (timer == NULL) {
        ZF_LOGE("Failed to allocate object of size %u\n", sizeof(seL4_timer_t));
        return NULL;
    }

    /* do hardware init */
    gpt_config_t config = {
        .id = gpt_id,
        .prescaler = prescaler,
        .vaddr = timer->vaddr
    };

    if (relative) {
        timer->timer = rel_gpt_get_timer(&config);
    } else {
        timer->timer = abs_gpt_get_timer(&config);
    }

    if (timer->timer == NULL) {
        timer_common_destroy(timer, vka, vspace);
        return NULL;
    }

    return timer;
}

DEPRECATED("use sel4platsupport_get_rel_gpt") seL4_timer_t *
sel4platsupport_get_gpt(vspace_t *vspace, simple_t *simple, vka_t *vka, seL4_CPtr notification,
                        gpt_id_t gpt_id, uint32_t prescaler)
{
    return sel4platsupport_get_rel_gpt(vspace, simple, vka, notification, gpt_id, 
                                       prescaler);
}

seL4_timer_t *
sel4platsupport_get_rel_gpt(vspace_t *vspace, simple_t *simple, vka_t *vka, seL4_CPtr notification,
                        gpt_id_t gpt_id, uint32_t prescaler)
{
    return sel4platsupport_get_gpt_impl(vspace, simple, vka, notification, gpt_id, 
                                       prescaler, true);
}

seL4_timer_t *
sel4platsupport_get_abs_gpt(vspace_t *vspace, simple_t *simple, vka_t *vka, seL4_CPtr notification,
                        gpt_id_t gpt_id, uint32_t prescaler)
{
    return sel4platsupport_get_gpt_impl(vspace, simple, vka, notification, gpt_id, 
                                       prescaler, false);
}

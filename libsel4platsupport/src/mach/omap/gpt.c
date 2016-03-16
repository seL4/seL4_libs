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
#include <assert.h>
#include <stdio.h>
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

    seL4_timer_t *timer = calloc(1, sizeof(seL4_timer_t));
    if (timer == NULL) {
        ZF_LOGE("Failed to allocate object of size %u\n", sizeof(seL4_timer_t));
        goto error;
    }

    /* find paddr/irq */
    void *paddr = omap_get_gpt_paddr(gpt_id);
    uint32_t irq = omap_get_gpt_irq(gpt_id);

    timer_common_data_t *data = timer_common_init(vspace, simple, vka, notification, irq, paddr);
    timer->data = data;

    if (timer->data == NULL) {
        goto error;
    }

    timer->handle_irq = timer_common_handle_irq;

    /* do hardware init */
    gpt_config_t config = {
        .id = gpt_id,
        .prescaler = prescaler,
        .vaddr = data->vaddr 
    };

    if (relative) {
        timer->timer = rel_gpt_get_timer(&config);
    } else {
        timer->timer = abs_gpt_get_timer(&config);
    }

    if (timer->timer == NULL) {
        goto error;
    }

    /* success */
    return timer;
error:
    if (timer != NULL) {
        timer_common_destroy(timer->data, vka, vspace);
        free(timer);
    }

    return NULL;
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
void
sel4platsupport_destroy_gpt(seL4_timer_t *timer, vka_t *vka, vspace_t *vspace)
{
    timer_stop(timer->timer);
    timer_common_destroy(timer->data, vka, vspace);
    free(timer);
}

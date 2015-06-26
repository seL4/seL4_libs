/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <vka/vka.h>
#include <vspace/vspace.h>
#include <simple/simple.h>
#include <sel4platsupport/timer.h>
#include <platsupport/plat/timer.h>
#include <utils/util.h>
#include <stdlib.h>
#include "../../timer_common.h"



seL4_timer_t *
sel4platsupport_get_timer(enum timer_id id, vka_t *vka, vspace_t *vspace,
                          simple_t *simple, seL4_CPtr aep)
{
    timer_common_data_t *data;
    seL4_timer_t *timer;
    /* Allocate the timer structure */
    timer = calloc(1, sizeof(*timer));
    if (timer == NULL) {
        LOG_ERROR("Failed to allocate object of size %u\n", sizeof(seL4_timer_t));
        return NULL;
    }
    /* init seL4 resources */
    timer->handle_irq = timer_common_handle_irq;
    data = timer_common_init(vspace, simple, vka, aep,
                             apq8064_timer_irqs[id],
                             (void*)apq8064_timer_paddrs[id]);
    timer->data = data;
    if (timer->data == NULL) {
        free(timer);
        return NULL;
    }

    /* do hardware init */
    timer_config_t config = {
        .vaddr = data->vaddr,
    };

    timer->timer = ps_get_timer(id, &config);
    if (timer->timer == NULL) {
        timer_common_destroy(timer->data, vka, vspace);
        free(timer);
        return NULL;
    }

    /* success */
    return timer;
}

seL4_timer_t *
sel4platsupport_get_default_timer(vka_t *vka, vspace_t *vspace, simple_t *simple,
                                  seL4_CPtr aep)
{
    return sel4platsupport_get_timer(TMR_DEFAULT, vka, vspace, simple, aep);
}

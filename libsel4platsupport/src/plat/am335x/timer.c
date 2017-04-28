/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdlib.h>
#include <vka/vka.h>
#include <simple/simple.h>
#include <sel4platsupport/timer.h>
#include <sel4platsupport/plat/timer.h>
#include <utils/util.h>
#include "../../timer_common.h"

seL4_timer_t *
sel4platsupport_get_timer(enum timer_id id, vka_t *vka, vspace_t *vspace,
                          simple_t *simple, seL4_CPtr notification)
{
    switch (id) {
    case DMTIMER2:
    case DMTIMER3:
    case DMTIMER4:
    case DMTIMER5:
    case DMTIMER6:
    case DMTIMER7:
        break;
    default:
        ZF_LOGE("Bad timer ID %d\n", id);
        return NULL;
    }

    seL4_timer_t *timer = timer_common_init(vspace, simple, vka, notification,
                                            dm_timer_irqs[id], (void*)dm_timer_paddrs[id]);
    if (timer == NULL) {
        return NULL;
    }

    timer_config_t config = {
        .vaddr = timer->vaddr,
        .irq = dm_timer_irqs[id],
    };

    timer->timer = ps_get_timer(id, &config);
    if (timer->timer == NULL) {
        timer_common_destroy(timer, vka, vspace);
        free(timer);
        return NULL;
    }

    /* success */
    return timer;
}

seL4_timer_t *
sel4platsupport_get_default_timer(vka_t *vka, vspace_t *vspace, simple_t *simple,
                                  seL4_CPtr notification)
{
    return sel4platsupport_get_timer(TMR_DEFAULT, vka, vspace, simple, notification);
}

int sel4platsupport_plat_init_default_timer_caps(UNUSED vka_t *vka, UNUSED vspace_t *vspace, UNUSED simple_t *simple, UNUSED timer_objects_t *timer_objects)
{
    return 0;
}

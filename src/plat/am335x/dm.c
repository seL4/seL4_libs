/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <sel4platsupport/device.h>

#include <platsupport/dm.h>
#include <stdio.h>

#include <assert.h>

#include <sel4utils/mapping.h>
#include <vka/object.h>

seL4_timer_t *
sel4platsupport_get_dm(vspace_t *vspace, simple_t *simple, vka_t *vka, seL4_CPtr aep) {

    seL4_timer_t *timer = calloc(1, sizeof(seL4_timer_t));
    if (timer == NULL) {
        LOG_ERROR("Failed to allocate object of size %u\n", sizeof(seL4_timer_t));
        goto error;
    }


    timer_common_data_t *data = timer_common_init(vspace, simple, vka, aep, irq, DMTIMER2_PADDR);
    timer->data = data;

    if (timer->data == NULL) {
        goto error;
    }

    timer->handle_irq = timer_common_handle_irq;
    timer->timer = dm_get_timer(data->vaddr);

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

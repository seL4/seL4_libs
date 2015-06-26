/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <platsupport/plat/pit.h>

#include <sel4platsupport/device.h>
#include <sel4platsupport/plat/pit.h>
#include <sel4platsupport/timer.h>

#include <utils/util.h>

#include <sel4utils/util.h>

#include "../../timer_common.h"

typedef struct {
    seL4_CPtr irq;
} seL4_pit_data_t;


static void
pit_handle_irq(seL4_timer_t *timer, UNUSED uint32_t irq)
{
    seL4_pit_data_t *data = (seL4_pit_data_t*) timer->data;
    timer_handle_irq(timer->timer, irq);
    seL4_IRQHandler_Ack(data->irq);
    /* pit handle irq actually does nothing */
}

seL4_timer_t *
sel4platsupport_get_pit(vka_t *vka, simple_t *simple, ps_io_port_ops_t *ops, seL4_CPtr aep)
{

    seL4_pit_data_t *data = malloc(sizeof(seL4_pit_data_t));
    if (data == NULL) {
        LOG_ERROR("Failed to allocate object of size %u\n", sizeof(*data));
        goto error;
    }


    seL4_timer_t *pit = calloc(1, sizeof(*pit));
    if (pit == NULL) {
        LOG_ERROR("Failed to malloc object of size %u\n", sizeof(*pit));
        goto error;
    }
    pit->handle_irq = pit_handle_irq;
    pit->data = data;

    /* set up irq */
    data->irq = timer_common_get_irq(vka, simple, PIT_INTERRUPT);
    if (data->irq == 0) {
        goto error;
    }

    /* bind to endpoint */
    if (seL4_IRQHandler_SetEndpoint(data->irq, aep) != seL4_NoError) {
        LOG_ERROR("seL4_IRQHandler_SetEndpoint failed\n");
        goto error;
    }

    /* ack (api hack) */
    seL4_IRQHandler_Ack(data->irq);

    /* finally set up the actual timer */
    pit->timer = pit_get_timer(ops);
    if (pit->timer == NULL) {
        goto error;
    }

    /* sucess */
    return pit;

error:
    if (data != NULL) {
        free(data);
    }
    if (data->irq != 0) {
        timer_common_cleanup_irq(vka, data->irq);
    }

    return NULL;
}

void
sel4platsupport_destroy_pit(seL4_timer_t *pit, vka_t *vka)
{

    timer_stop(pit->timer);
    seL4_pit_data_t *data = (seL4_pit_data_t *) pit->data;
    timer_common_cleanup_irq(vka, data->irq);
    free(data);
    free(pit);
}

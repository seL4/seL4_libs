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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <platsupport/plat/pit.h>

#include <sel4platsupport/arch/io.h>
#include <sel4platsupport/device.h>
#include <sel4platsupport/plat/pit.h>
#include <sel4platsupport/timer.h>

#include <utils/util.h>

#include <utils/util.h>

#include "../../timer_common.h"

static void
pit_destroyer(seL4_timer_t *pit, vka_t *vka, UNUSED vspace_t *vspace)
{

    timer_stop(pit->timer);
    timer_common_cleanup_irq(vka, pit->irq);
    free(pit->data);
    free(pit);
}

seL4_timer_t *
sel4platsupport_get_pit(vka_t *vka, simple_t *simple, ps_io_port_ops_t *ops, seL4_CPtr notification)
{
    seL4_timer_t *pit = calloc(1, sizeof(*pit));
    if (pit == NULL) {
        ZF_LOGE("Failed to malloc object of size %zu\n", sizeof(*pit));
        return NULL;
    }

    /* initialise an io port ops interface if none was provided */
    if (ops == NULL) {
        ops = calloc(1, sizeof(*ops));

        if (ops == NULL) {
            ZF_LOGE("Failed to malloc object of size %zu", sizeof(*ops));
            goto error;
        }

        /* store ops pointer in pit so it can be freed later */
        pit->data = ops;

        if (sel4platsupport_get_io_port_ops(ops, simple) != 0) {
            ZF_LOGE("Failed to initialise io port ops");
            goto error;
        }
    } else {
        pit->data = NULL;
    }

    pit->destroy = pit_destroyer;
    pit->handle_irq = timer_common_handle_irq;

    /* set up irq */
    cspacepath_t dest;
    if (sel4platsupport_copy_irq_cap(vka, simple, PIT_INTERRUPT, &dest) != seL4_NoError) {
        goto error;
    }
    pit->irq = dest.capPtr;

    /* bind to endpoint */
    if (seL4_IRQHandler_SetNotification(pit->irq, notification) != seL4_NoError) {
        ZF_LOGE("seL4_IRQHandler_SetEndpoint failed\n");
        goto error;
    }

    /* ack (api hack) */
    seL4_IRQHandler_Ack(pit->irq);

    /* finally set up the actual timer */
    pit->timer = pit_get_timer(ops);
    if (pit->timer == NULL) {
        goto error;
    }

    /* sucess */
    return pit;

error:
    if (pit->irq != 0) {
        timer_common_cleanup_irq(vka, pit->irq);
        free(pit->data);
        free(pit);
    }

    return NULL;
}

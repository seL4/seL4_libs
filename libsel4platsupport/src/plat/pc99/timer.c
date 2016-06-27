/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <platsupport/io.h>
#include <vka/vka.h>
#include <vspace/vspace.h>
#include <sel4platsupport/plat/timer.h>
#include <sel4platsupport/arch/io.h>
#include <simple/simple.h>
#include <stdint.h>
#include <stdlib.h>
#include <utils/util.h>

static ps_io_port_ops_t ops;

seL4_timer_t *sel4platsupport_get_default_timer(vka_t *vka, UNUSED vspace_t *vspace,
                                                simple_t *simple, seL4_CPtr notification)
{

    UNUSED int error = sel4platsupport_get_io_port_ops(&ops, simple);
    assert(error == 0);

    seL4_timer_t *timer = sel4platsupport_get_hpet(vspace, simple, NULL, vka, notification, 
                                                   MSI_MIN);
    assert(timer != NULL);

    return timer;
}

static seL4_timer_t *
finish_get_tsc(seL4_timer_t *seL4_timer, pstimer_t *ps_timer)
{
    if (ps_timer == NULL) {
        ZF_LOGE("Failed to init tsc");
        free(seL4_timer);
        return NULL;
    }

    seL4_timer->timer = ps_timer;
    return seL4_timer;
}

seL4_timer_t *
sel4platsupport_get_tsc_timer_freq(uint64_t freq)
{
    if (freq == 0) {
        ZF_LOGE("Can't init tsc with 0 freq");
        return NULL;
    }

    seL4_timer_t *timer = calloc(1, sizeof(seL4_timer_t));
    if (timer == NULL) {
        ZF_LOGE("Failed to allocate timer");
        return NULL;
    }

    return finish_get_tsc(timer, tsc_get_timer_with_freq(freq));
}

seL4_timer_t *
sel4platsupport_get_tsc_timer(seL4_timer_t *timeout_timer)
{
    if (timeout_timer == NULL) {
        ZF_LOGE("Cannot initialise tsc frequency, timeout timer is NULL");
        return NULL;
    }

    seL4_timer_t *timer = calloc(1, sizeof(seL4_timer_t));
    if (timer == NULL) {
        ZF_LOGE("Failed to allocate timer");
        return NULL;
    }

    return finish_get_tsc(timer, tsc_get_timer(timeout_timer->timer));
}


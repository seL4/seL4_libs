/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(D61_BSD)
 */

#pragma once

#include <platsupport/timer.h>
#include <simple/simple.h>
#include <vka/vka.h>
#include <vka/object.h>
#include <vspace/vspace.h>

typedef struct seL4_timer seL4_timer_t;

typedef void (*handle_irq_fn_t)(seL4_timer_t *timer, uint32_t irq);
typedef void (*destroy_fn_t)(seL4_timer_t *timer, vka_t *vka, vspace_t *vspace);

struct seL4_timer {
    /* os independant timer interface */
    pstimer_t *timer;

    /* frame object for timer frame */
    vka_object_t frame;
    /* irq cap */
    seL4_CPtr irq;
    /* vaddr that frame is mapped in at */
    void *vaddr;

    /* timer specific config data. View in platsupport/plat/<timer>.h */
    void *data;

    /* sel4 specific functions to call to deal with timer */
    handle_irq_fn_t handle_irq;

    /* destroy this timer, it will no longer be valid */
    destroy_fn_t destroy;
};

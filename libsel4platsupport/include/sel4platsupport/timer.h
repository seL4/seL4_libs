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

#pragma once

#include <sel4platsupport/pmem.h>
#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <sel4platsupport/irq.h>
#include <simple/simple.h>
#include <vka/vka.h>
#include <vka/object.h>
#include <vspace/vspace.h>

#define MAX_IRQS 4
#define MAX_OBJS 4

typedef struct seL4_timer seL4_timer_t;

typedef void (*handle_irq_fn_t)(seL4_timer_t *timer, uint32_t irq);
typedef void (*destroy_fn_t)(seL4_timer_t *timer, vka_t *vka, vspace_t *vspace);

typedef struct timer_objects {
    /* this struct is copied using c's struct deep copy semantics.
     * avoid adding pointers to it */
    size_t nirqs;
    sel4ps_irq_t irqs[MAX_IRQS];
    size_t nobjs;
    sel4ps_pmem_t objs[MAX_OBJS];
} timer_objects_t;

struct seL4_timer {
    /* os independent timer interface */
    ltimer_t ltimer;

    /* objects allocated for this timer */
    timer_objects_t to;

    /* sel4 specific functions to call to deal with timer */
    handle_irq_fn_t handle_irq;

    /* destroy this timer, it will no longer be valid */
    destroy_fn_t destroy;
};

/**
 * Creates caps required for calling sel4platsupport_get_default_timer and stores them
 * in supplied timer_objects_t.
 *
 * Uses the provided vka, vspace and simple interfaces for creating the caps.
 * (A vspace is required because on some platforms devices may need to be mapped in
 *  order to query hardware features.)
 *
 * @param  vka           Allocator to allocate objects with
 * @param  vspace        vspace for mapping device frames into memory if necessary.
 * @param  simple        simple interface for access to init caps
 * @param  timer_objects struct for returning cap meta data.
 * @return               0 on success, otherwise failure.
 */
int sel4platsupport_init_default_timer_caps(vka_t *vka, vspace_t *vspace, simple_t *simple, timer_objects_t *timer_objects);

/*
 * Initialise the default timer for this platform and do all of the seL4 related work.
 *
 * After this operation is complete, the irqs from the ltimer will be delivered to the
 * provided notification object with badges of BIT(seL4_BadgeBits - 1) - n where n is the index
 * of the irq in the default ltimer.
 *
 * The passed ops interface is retained by the timer and must stay valid until the timer is destroyed
 *
 * @param vka          an initialised vka implementation that can allocate the physical
 *                     addresses (if any) required by the ltimer.
 * @param vspace       an initialised vspace for manging virtual memory.
 * @param simple       for getting irq capabilities for the ltimer irqs.
 * @param ops          I/O ops interface for interacting with hardware resources
 * @param ntfn         notification object capability for irqs to be delivered to.
 * @param timer        a timer structure to initialise.
 * @return             0 on success.
 */
int sel4platsupport_init_default_timer_ops(vka_t *vka, vspace_t *vspace, simple_t *simple, ps_io_ops_t ops,
                                seL4_CPtr notification, seL4_timer_t *timer);

/*
 * Wrapper around sel4platsupport_init_default_timer_ops that generates as much of an
 * io_ops interface as it can from the given vka and vspace
 */
int sel4platsupport_init_default_timer(vka_t *vka, vspace_t *vspace, simple_t *simple,
                                seL4_CPtr notification, seL4_timer_t *timer);
/*
 * Initialise an seL4_timer with irqs from provided timer objects. As per sel4platsupport_init_timer,
 * timer_objects are provided and the user is expected to initialise the ltimer after calling this function
 * (to avoid unacked irqs due to irq caps not being set up).
 *
 * This function just does the seL4 parts of the timer init.
 *
 *
 * @param vka          an initialised vka implementation that can allocate the physical
 *                     address returned by sel4platsupport_get_default_timer_paddr.
 * @param simple       for getting irq capabilities for the ltimer irqs.
 * @param ntfn         notification object capability for irqs to be delivered to.
 * @param timer        a timer structure to populate.
 * @param objects      the timer objects that this timer needs to initialise.
 * @return             0 on success.
 */
int sel4platsupport_init_timer_irqs(vka_t *vka, simple_t *simple, seL4_CPtr ntfn,
        seL4_timer_t *timer, timer_objects_t *objects);

/*
 * Handle a timer irq for this timer.
 *
 * @param timer       initialised timer.
 * @param badge       badge recieved on the notification object this timer was initialised with.
 */
void sel4platsupport_handle_timer_irq(seL4_timer_t *timer, seL4_Word badge);

/*
 * Destroy the timer, freeing any resources allocated during initialisation.
 *
 * @param timer       initialised timer.
 * @param vka         vka used to initialise the timer.
 *
 */
void sel4platsupport_destroy_timer(seL4_timer_t *timer, vka_t *vka);

/*
 * Helper function for getting the nth irq cap out of the timer_objects struct if its type is
 * known.  This is often used for initialising an arch_simple interface.
 *
 * @param to       timer_objects struct containing irq caps.
 * @param id       index of irq in the struct.
 * @param type     Interrupt type
 *
 * @return         IRQ cap on success, otherwise seL4_CapNull on failure.
 */
seL4_CPtr sel4platsupport_timer_objs_get_irq_cap(timer_objects_t *to, int id, irq_type_t type);
/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/*
 * Note that you cannot use periodic and oneshot timeouts simultaneously.
 */
#ifndef __SEL4_PLAT_SUPPORT_EPIT_H
#define __SEL4_PLAT_SUPPORT_EPIT_H

#include <autoconf.h>

#include <stdint.h>
#include <sel4platsupport/timer.h>
#include <platsupport/plat/epit_constants.h>

#include <vspace/vspace.h>
#include <vka/vka.h>
#include <simple/simple.h>

#define DEFAULT_TIMER_PADDR EPIT2_DEVICE_PADDR
#define DEFAULT_TIMER_INTERRUPT EPIT2_INTERRUPT


typedef enum {
    EPIT1,
    EPIT2
} epit_id_t;


typedef timer_common_data_t epit_data;

/**
 * Get an interface for an initialised epit timer.
 *
 * @param aep async endpoint for the irq to come in on
 * @param prescaler to scale time by. 0 = divide by 1. 1 = divide by 2, ...
 * @param epit_id some platforms have more than one epit, pick one.
 */
seL4_timer_t *sel4platsupport_get_epit(vspace_t *vspace, simple_t *simple, vka_t *vka,
        seL4_CPtr aep, uint32_t prescaler, epit_id_t id);


void sel4platsupport_destroy_epit(seL4_timer_t *timer, vka_t *vka, vspace_t *vspace);

#endif /* __SEL4_PLAT_SUPPORT_EPIT_H */

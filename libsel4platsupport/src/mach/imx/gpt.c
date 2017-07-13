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

#include <autoconf.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sel4platsupport/timer.h>
#include <sel4platsupport/mach/gpt.h>
#include <platsupport/mach/gpt.h>
#include <utils/util.h>
#include "../../timer_common.h"

seL4_timer_t *
sel4platsupport_get_gpt(vspace_t *vspace, simple_t *simple, vka_t *vka, seL4_CPtr notification,
                        uint32_t prescaler)
{

    void *paddr =  (void *) GPT1_DEVICE_PADDR;
    uint32_t irq = GPT1_INTERRUPT;

    seL4_timer_t *timer = timer_common_init(vspace, simple, vka, notification, irq, paddr);
    if (timer == NULL) {
        ZF_LOGE("Failed to allocate object of size %u\n", sizeof(seL4_timer_t));
        return NULL;
    }

    /* do hardware init */
    gpt_config_t config = {
        .vaddr = timer->vaddr,
        .prescaler = prescaler
    };

    timer->timer = gpt_get_timer(&config);
    if (timer->timer == NULL) {
        timer_common_destroy(timer, vka, vspace);
        return NULL;
    }

    /* success */
    return timer;
}

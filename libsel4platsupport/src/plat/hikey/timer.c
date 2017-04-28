/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdlib.h>
#include <vka/vka.h>
#include <vka/object.h>
#include <simple/simple.h>
#include <sel4platsupport/timer.h>
#include <sel4platsupport/device.h>
#include <sel4platsupport/plat/timer.h>
#include <utils/util.h>
#include "../../timer_common.h"

typedef struct sel4_hikey_vupcounter_privdata {
    seL4_timer_t *dualtimer2_sel4timer;
    seL4_timer_t *rtc0_sel4timer;
    timer_config_t config;
} sel4_hikey_vupcounter_privdata_t;

static seL4_timer_t *sel4_hikey_vupcounter;

int sel4platsupport_plat_init_default_timer_caps(vka_t *vka, UNUSED vspace_t *vspace, simple_t *simple, timer_objects_t *timer_objects)
{
    int error;
    plat_timer_objects_t *plat_timer_objects = &timer_objects->arch_timer_objects.plat_timer_objects;

    /* Get the hikey RTC0 timestamp device's device-ut. We don't need an IRQ cap
     * for it because we don't use the RTC's "alarm" IRQ feature.
     */
    plat_timer_objects->clock_timer_paddr = RTC0_PADDR;
    error = vka_alloc_untyped_at(vka, seL4_PageBits, plat_timer_objects->clock_timer_paddr,
                                 &plat_timer_objects->clock_timer_dev_ut_obj);
    if (error) {
        ZF_LOGF("Failed to obtain device-ut cap for RTC0 wall-clock "
        "timer.");
        return error;
    }

    /* Also get another of the dual timers to use as the accompanying
     * downcounter for the virtual upcounter.
     */
    plat_timer_objects->extra_timer_paddr = DMTIMER2_PADDR;
    error = vka_alloc_untyped_at(vka, seL4_PageBits, plat_timer_objects->extra_timer_paddr,
                                 &plat_timer_objects->extra_timer_dev_ut_obj);
    if (error) {
        ZF_LOGF("Failed to obtain device-ut cap for DMTIMER2 downcounter "
        "timer.");
        return error;
    }
    /* Alloc space for the IRQ caps for the IRQs of both timers. */
    error = vka_cspace_alloc_path(vka, &plat_timer_objects->clock_irq_path);
    if (error) {
        ZF_LOGF("Failed to allocate RTC0 timer IRQ.");
        return error;
    }
    error = vka_cspace_alloc_path(vka, &plat_timer_objects->extra_timer_irq_path);
    if (error) {
        ZF_LOGF("Failed to allocate DMTIMER2 timer IRQ.");
        return error;
    }
    /* Obtain caps to the IRQs. */
    error = sel4platsupport_copy_irq_cap(vka, simple,
                                         RTC0_INTERRUPT,
                                         &plat_timer_objects->clock_irq_path);
    if (error) {
        ZF_LOGF("Failed to obtain RTC0 timer IRQ cap.");
        return error;
    }
    error = sel4platsupport_copy_irq_cap(vka, simple,
                                         DMTIMER2_INTERRUPT,
                                         &plat_timer_objects->extra_timer_irq_path);
    if (error) {
        ZF_LOGF("Failed to obtain DMTIMER2 timer IRQ cap.");
        return error;
    }
    return 0;
}

static void
sel4platsupport_hikey_vupcounter_destroy(seL4_timer_t *timer,
                                  vka_t *vka, vspace_t *vspace)
{
    if (timer->timer) {
        timer_stop(timer->timer);
    }

    if (timer->data) {
        sel4_hikey_vupcounter_privdata_t *priv = timer->data;

        if (priv->rtc0_sel4timer) {
            timer_common_destroy(priv->rtc0_sel4timer, vka, vspace);
        }
        if (priv->dualtimer2_sel4timer) {
            timer_common_destroy(priv->dualtimer2_sel4timer, vka, vspace);
        }
    }

    free(timer->data);
    free(timer);
    sel4_hikey_vupcounter = NULL;
}

static void
sel4platsupport_hikey_vupcounter_handle_irq(UNUSED seL4_timer_t *timer,
                                     UNUSED uint32_t irq)
{
    /* Nothing to do */
}

/* See the documentation for this function in plat/hikey/timer.h of seL4_libs'
 * libsel4platsupport.
 */
seL4_timer_t *
sel4platsupport_hikey_get_vupcounter_timer(vka_t *vka, vspace_t *vspace,
                                           simple_t *simple,
                                           UNUSED seL4_CPtr notification)
{
    int error;
    pstimer_t *ps_hikey_vupcounter;
    vka_object_t rtc0_irq_obj, dualtimer2_irq_obj;
    sel4_hikey_vupcounter_privdata_t *vupcounter_priv;

    /* If it's already been initialized... */
    if (sel4_hikey_vupcounter != NULL && sel4_hikey_vupcounter->timer != NULL) {
        return sel4_hikey_vupcounter;
    }

    sel4_hikey_vupcounter = calloc(sizeof(*sel4_hikey_vupcounter), 1);
    if (sel4_hikey_vupcounter == NULL) {
        return NULL;
    }

    vupcounter_priv = calloc(sizeof(*vupcounter_priv), 1);
    if (vupcounter_priv == NULL) {
        free(sel4_hikey_vupcounter);
        sel4_hikey_vupcounter = NULL;
    }

    sel4_hikey_vupcounter->data = vupcounter_priv;
    sel4_hikey_vupcounter->handle_irq = &sel4platsupport_hikey_vupcounter_handle_irq;
    sel4_hikey_vupcounter->destroy = &sel4platsupport_hikey_vupcounter_destroy;

    /* Get IRQ caps for both devices. */
    error = vka_alloc_notification(vka, &rtc0_irq_obj);
    if (error != 0) {
        ZF_LOGE("Failed to vka_alloc_notification for RTC0 timer.");
        return NULL;
    }

    error = vka_alloc_notification(vka, &dualtimer2_irq_obj);
    if (error != 0) {
        ZF_LOGE("Failed to vka_alloc_notification for DMTIMER2 timer.");
        sel4platsupport_hikey_vupcounter_destroy(sel4_hikey_vupcounter, vka, vspace);
        return NULL;
    }

    /* Now initialize both the other timer devices, and then finally, initialize
     * the vupcounter device.
     *
     * RTC0 first, then DMTIMER2.
     */
    vupcounter_priv->rtc0_sel4timer = sel4platsupport_get_timer(RTC0, vka, vspace, simple,
                                               rtc0_irq_obj.cptr);
    if (vupcounter_priv->rtc0_sel4timer == NULL) {
        ZF_LOGE("Failed to sel4platsupport_get_timer for the RTC0 device.");
        sel4platsupport_hikey_vupcounter_destroy(sel4_hikey_vupcounter, vka, vspace);
        return NULL;
    }

    vupcounter_priv->dualtimer2_sel4timer = sel4platsupport_get_timer(DMTIMER2,
                                                     vka, vspace, simple,
                                                     dualtimer2_irq_obj.cptr);
    if (vupcounter_priv->rtc0_sel4timer == NULL) {
        ZF_LOGE("Failed to sel4platsupport_get_timer for the DMTIMER2 device.");
        sel4platsupport_hikey_vupcounter_destroy(sel4_hikey_vupcounter, vka, vspace);
        return NULL;
    }

    /* Next, pass the pstimer_t* handles within the seL4_timer_t instances to
     * platsupport and ask for a handle to the virtual upcounter device.
     */
    vupcounter_priv->config.rtc_id = RTC0;
    vupcounter_priv->config.dualtimer_id = DMTIMER2;
    vupcounter_priv->config.vupcounter_config.rtc_timer = vupcounter_priv->
                                                          rtc0_sel4timer->timer;
    vupcounter_priv->config.vupcounter_config.dualtimer_timer = vupcounter_priv->
                                                                dualtimer2_sel4timer->timer;

    ps_hikey_vupcounter = ps_get_timer(VIRTUAL_UPCOUNTER, &vupcounter_priv->config);
    if (ps_hikey_vupcounter == NULL) {
        sel4platsupport_hikey_vupcounter_destroy(sel4_hikey_vupcounter, vka, vspace);
        ZF_LOGE("Failed to ps_get_timer for the VIRTUAL_UPCOUNTER device.");
        return NULL;
    }

    /* Wrap the pstimer_t inside of a seL4_timer_t */
    sel4_hikey_vupcounter->timer = ps_hikey_vupcounter;
    return sel4_hikey_vupcounter;
}

seL4_timer_t *
sel4platsupport_get_timer(enum timer_id id, vka_t *vka, vspace_t *vspace,
                          simple_t *simple, seL4_CPtr notification)
{
    assert(vka != NULL && vspace != NULL && simple != NULL);

    if (id >= NUM_TIMERS || id < 0) {
        ZF_LOGE("Invalid timer device ID %d.", id);
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


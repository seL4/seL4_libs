/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include "timer_common.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <utils/util.h>
#include <vka/capops.h>
#include <sel4platsupport/device.h>

void
timer_common_destroy_frame(seL4_timer_t *timer, vka_t *vka, vspace_t *vspace)
{
    if (timer == NULL) {
        return;
    }

    if (timer->vaddr != NULL) {
        vspace_unmap_pages(vspace, timer->vaddr, 1, seL4_PageBits, VSPACE_PRESERVE);
    }
    if (timer->frame.cptr != 0) {
        vka_free_object(vka, &timer->frame);
    }
}

static void
timer_common_destroy_internal(seL4_timer_t *timer, vka_t *vka, vspace_t *vspace)
{
    timer_common_destroy_frame(timer, vka, vspace);
    if (timer->irq != 0) {
        seL4_IRQHandler_Clear(timer->irq);
        timer_common_cleanup_irq(vka, timer->irq);
    }
    free(timer);
}

void
timer_common_handle_irq(seL4_timer_t *timer, uint32_t irq)
{
    timer_handle_irq(timer->timer, irq);
    int error = seL4_IRQHandler_Ack(timer->irq);
    if (error != seL4_NoError) {
        ZF_LOGE("Failed to ack irq %d, error %d", irq, error);
    }
}

void
timer_common_cleanup_irq(vka_t *vka, seL4_CPtr irq)
{
    cspacepath_t path;
    vka_cspace_make_path(vka, irq, &path);
    vka_cnode_delete(&path);
    vka_cspace_free(vka, irq);
}

int
timer_common_init_frame(seL4_timer_t *timer, vspace_t *vspace, vka_t *vka, uintptr_t paddr)
{
    int error = sel4platsupport_alloc_frame_at(vka, (uintptr_t) paddr, seL4_PageBits,
                                           &timer->frame);
    if (error != seL4_NoError) {
        return -1;
    }

    /* map in the frame */
    timer->vaddr = vspace_map_pages(vspace, &timer->frame.cptr,
            (uintptr_t *) &timer->frame, seL4_AllRights, 1, seL4_PageBits, 0);
    ZF_LOGV("Mapped timer at %p\n", timer->vaddr);
    if (timer->vaddr == NULL) {
        timer_common_destroy_frame(timer, vka, vspace);
        ZF_LOGE("Failed to map page at vaddr %p\n", timer->vaddr);
        return -1;
    }

    return 0;
}

seL4_timer_t *
timer_common_init(vspace_t *vspace, simple_t *simple,
                  vka_t *vka, seL4_CPtr notification, uint32_t irq_number, void *paddr)
{
    seL4_timer_t *timer = calloc(1, sizeof(seL4_timer_t));
    if (timer == NULL) {
        return NULL;
    }

    /* default functions - users can override */
    timer->handle_irq = timer_common_handle_irq;
    timer->destroy = timer_common_destroy;

    if (timer_common_init_frame(timer, vspace, vka, (uintptr_t) paddr)) {
        timer_common_destroy(timer, vka, vspace);
        return NULL;
    }

    cspacepath_t path;
    if (sel4platsupport_copy_irq_cap(vka, simple, irq_number, &path)) {
        timer_common_destroy(timer, vka, vspace);
        return NULL;
    }
    timer->irq = path.capPtr;

    /* set the endpoint */
    int error = seL4_IRQHandler_SetNotification(timer->irq, notification);
    if (error != seL4_NoError) {
        ZF_LOGE("seL4_IRQHandler_SetNotification failed with error %d\n", error);
        timer_common_destroy(timer, vka, vspace);
        return NULL;
    }

    ZF_LOGV("Timer initialised\n");
    /* success */
    return timer;
}

void
timer_common_destroy(seL4_timer_t *timer, vka_t *vka, vspace_t *vspace)
{
    if (timer->timer) {
        timer_stop(timer->timer);
    }
    timer_common_destroy_internal(timer, vka, vspace);
}

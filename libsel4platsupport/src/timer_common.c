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
#include <string.h>
#include <stdlib.h>
#include <utils/util.h>
#include <vka/capops.h>
#include <sel4platsupport/device.h>

#ifdef CONFIG_LIB_SEL4_VSPACE

void
timer_common_handle_irq(seL4_timer_t *timer, uint32_t irq)
{
    timer_common_data_t *data = (timer_common_data_t *) timer->data;
    timer_handle_irq(timer->timer, irq);
    int error = seL4_IRQHandler_Ack(data->irq);
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

timer_common_data_t *
timer_common_init(vspace_t *vspace, simple_t *simple,
                  vka_t *vka, seL4_CPtr notification, uint32_t irq_number, void *paddr)
{
    int error;
    timer_common_data_t *timer_data = NULL;

    timer_data = malloc(sizeof(timer_common_data_t));
    if (timer_data == NULL) {
        LOG_ERROR("Failed to allocate timer_common_data_t size: %zu\n", sizeof(timer_common_data_t));
        goto error;
    }
    memset(timer_data, 0, sizeof(timer_common_data_t));
 
    error = sel4platsupport_copy_frame_cap(vka, simple, paddr, seL4_PageBits, &timer_data->frame);
    if (error != seL4_NoError) {
        goto error;
    }

    /* map in the frame */
    timer_data->vaddr = vspace_map_pages(vspace, &timer_data->frame.capPtr, NULL, seL4_AllRights,
                                         1, seL4_PageBits, 0);
    ZF_LOGV("Mapped timer at %p\n", timer_data->vaddr);
    if (timer_data->vaddr == NULL) {
        LOG_ERROR("Failed to map page at vaddr %p\n", timer_data->vaddr);
        goto error;
    }

    cspacepath_t path;
    error = sel4platsupport_copy_irq_cap(vka, simple, irq_number, &path);
    timer_data->irq = path.capPtr;
    if (error != seL4_NoError) {
        goto error;
    }

    /* set the end point */
    error = seL4_IRQHandler_SetNotification(timer_data->irq, notification);
    if (error != seL4_NoError) {
        LOG_ERROR("seL4_IRQHandler_SetNotification failed with error %d\n", error);
        goto error;
    }

    ZF_LOGV("Timer initialised\n");
    /* success */
    return timer_data;

error:
    /* clean up on failure */
    timer_common_destroy(timer_data, vka, vspace);

    return NULL;
}

void
timer_common_destroy(timer_common_data_t *timer_data, vka_t *vka, vspace_t *vspace)
{
    if (timer_data != NULL) {

        if (timer_data->irq != 0) {
            seL4_IRQHandler_Clear(timer_data->irq);
            cspacepath_t irq_path;
            vka_cspace_make_path(vka, timer_data->irq, &irq_path);
            vka_cnode_delete(&irq_path);
            vka_cspace_free(vka, timer_data->irq);
        }
        if (timer_data->vaddr != NULL) {
            vspace_unmap_pages(vspace, timer_data->vaddr, 1, seL4_PageBits, VSPACE_PRESERVE);
        }
        if (timer_data->frame.capPtr != 0) {
            vka_cnode_delete(&timer_data->frame);
            vka_cspace_free(vka, timer_data->frame.capPtr);
        }
        free(timer_data);
    }
}

#endif /* CONFIG_LIB_SEL4_VSPACE */

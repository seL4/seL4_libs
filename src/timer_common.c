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
#include <sel4utils/util.h>
#include <vka/capops.h>

#ifdef CONFIG_LIB_SEL4_VSPACE

seL4_CPtr
timer_common_get_irq(vka_t *vka, simple_t *simple, uint32_t irq_number)
{
    seL4_CPtr irq;

    /* allocate a cslot for the irq cap */
    int error = vka_cspace_alloc(vka, &irq);
    if (error != 0) {
        LOG_ERROR("Failed to allocate cslot for irq\n");
        return 0;
    }

    cspacepath_t path;
    vka_cspace_make_path(vka, irq, &path);

    error = simple_get_IRQ_control(simple, irq_number, path);
    if  (error != seL4_NoError) {
        LOG_ERROR("Failed to get cap to irq_number %u\n", irq_number);
        vka_cspace_free(vka, irq);
        return 0;
    }

    return irq;
}

void
timer_common_handle_irq(seL4_timer_t *timer, uint32_t irq)
{
    timer_common_data_t *data = (timer_common_data_t *) timer->data;
    timer_handle_irq(timer->timer, irq);
    seL4_IRQHandler_Ack(data->irq);
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
                  vka_t *vka, seL4_CPtr aep, uint32_t irq_number, void *paddr)
{
    int error;
    timer_common_data_t *timer_data = NULL;

    timer_data = malloc(sizeof(timer_common_data_t));
    if (timer_data == NULL) {
        LOG_ERROR("Failed to allocate timer_common_data_t size: %u\n", sizeof(timer_common_data_t));
        goto error;
    }
    memset(timer_data, 0, sizeof(timer_common_data_t));

    /* allocate a cslot for the frame */
    seL4_CPtr frame_cap;
    error = vka_cspace_alloc(vka, &frame_cap);
    if (error) {
        LOG_ERROR("Failed to allocate cslot for frame cap\n");
        goto error;
    }

    /* find the physical frame */
    vka_cspace_make_path(vka, frame_cap, &timer_data->frame);
    error = simple_get_frame_cap(simple, paddr, seL4_PageBits, &timer_data->frame);
    if (error) {
        LOG_ERROR("Failed to find frame at paddr %p\n", paddr);
        goto error;
    }

    /* map in the frame */
    timer_data->vaddr = vspace_map_pages(vspace, &frame_cap, NULL, seL4_AllRights, 1, seL4_PageBits, 0);
    if (timer_data->vaddr == NULL) {
        LOG_ERROR("Failed to map page at vaddr %p\n", timer_data->vaddr);
        goto error;
    }

    timer_data->irq = timer_common_get_irq(vka, simple, irq_number);
    if (timer_data->irq == 0) {
        goto error;
    }

    /* set the end point */
    error = seL4_IRQHandler_SetEndpoint(timer_data->irq, aep);
    if (error != seL4_NoError) {
        LOG_ERROR("seL4_IRQHandler_SetEndpoint failed with error %d\n", error);
        goto error;
    }

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

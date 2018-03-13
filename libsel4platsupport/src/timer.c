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
#include <vka/object.h>
#include <vka/vka.h>
#include <vka/capops.h>
#include <sel4platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <sel4platsupport/device.h>
#include <sel4platsupport/io.h>
#include <utils/util.h>

static void cleanup_timer_irq(vka_t *vka, sel4ps_irq_t *irq)
{
    seL4_IRQHandler_Clear(irq->handler_path.capPtr);
    /* clear the cslots */
    vka_cnode_delete(&irq->badged_ntfn_path);
    vka_cnode_delete(&irq->handler_path);
    /* free the cslots */
    vka_cspace_free(vka, irq->badged_ntfn_path.capPtr);
    vka_cspace_free(vka, irq->handler_path.capPtr);
}


void sel4platsupport_destroy_timer(seL4_timer_t *timer, vka_t *vka)
{
    ltimer_destroy(&timer->ltimer);
    assert(timer->to.nirqs < MAX_IRQS);
    for (size_t i = 0; i < timer->to.nirqs; i++) {
        cleanup_timer_irq(vka, &timer->to.irqs[i]);
    }

    assert(timer->to.nobjs < MAX_OBJS);
    for (size_t i = 0; i < timer->to.nobjs; i++) {
        vka_free_object(vka, &timer->to.objs[i].obj);
    }
}

void sel4platsupport_handle_timer_irq(seL4_timer_t *timer, seL4_Word badge)
{
    assert(timer->to.nirqs < MAX_IRQS);
    /* check which bits are set to find which irq to handle */
    for (unsigned long i = 0; badge && i < timer->to.nirqs; i++) {
        /* invert the bit: we take the top badge bits to identify timers */
        long irq = seL4_BadgeBits - i - 1;
        if (badge & BIT(irq)) {
            /* mask the bit out of the badge */
            badge &= ~BIT(irq);
            if (timer->to.irqs[i].irq.type != PS_NONE) {
                ltimer_handle_irq(&timer->ltimer, &timer->to.irqs[i].irq);
                int error = seL4_IRQHandler_Ack(timer->to.irqs[i].handler_path.capPtr);
                if (error) {
                    ZF_LOGE("Failed to ack irq %lu, error %d", irq, error);
                }
            }
        }
    }
}

static int setup_irq(vka_t *vka, sel4ps_irq_t *irq, seL4_Word badge,
        seL4_CPtr ntfn)
{
    int error = vka_cspace_alloc_path(vka, &irq->badged_ntfn_path);
    cspacepath_t path;
    vka_cspace_make_path(vka, ntfn, &path);
    if (!error) {
        /* badge it */
        error = vka_cnode_mint(&irq->badged_ntfn_path, &path,
                seL4_AllRights, badge);
    }
    if (!error) {
        /* set notification *before* acking any pending IRQ to ensure there is no race where
         * we lose an IRQ */
        error =  seL4_IRQHandler_SetNotification(irq->handler_path.capPtr, irq->badged_ntfn_path.capPtr);
    }
    if (!error) {
        error = seL4_IRQHandler_Ack(irq->handler_path.capPtr);
    }
    return error;
}

static int get_nth_pmem(vka_t *vka, ltimer_t *ltimer, sel4ps_pmem_t *obj, int n)
{
    int error = ltimer_get_nth_pmem(ltimer, n, &obj->region);
    if (!error && obj->region.length > PAGE_SIZE_4K) {
        ZF_LOGW("Support for timers with anything but 4K pages unimplemented! length %llu",
                obj->region.length);
        return ENOSYS;
    }

    obj->region.length = PAGE_SIZE_4K;
    if (!error) {
        error = vka_alloc_untyped_at(vka, seL4_PageBits, obj->region.base_addr,
                                     &obj->obj);
    }
    obj->obj.size_bits = seL4_PageBits;
    if (error) {
        ZF_LOGF("Failed to obtain device-ut cap for default timer.");
    }
    return error;
}

static inline size_t get_nirqs(ltimer_t *ltimer)
{
    size_t nirqs = ltimer_get_num_irqs(ltimer);
    if (nirqs > MAX_IRQS) {
        ZF_LOGE("MAX_IRQS insufficient for timer");
        return -1;
    }

    return nirqs;
}

static int init_timer_internal(vka_t *vka, simple_t *simple, seL4_CPtr ntfn, seL4_timer_t *timer, size_t nirqs)
{

    /* set up the irq caps the timer needs */
    timer->to.nirqs = 0;
    for (size_t i = 0; i < nirqs; i++) {
        assert(timer->to.irqs[i].irq.type != PS_NONE);
        int error = sel4platsupport_copy_irq_cap(vka, simple, &timer->to.irqs[i].irq, &timer->to.irqs[i].handler_path);
        if (!error) {
            error = setup_irq(vka, &timer->to.irqs[i], BIT(seL4_BadgeBits - i - 1), ntfn);
        }
        if (error) {
            sel4platsupport_destroy_timer(timer, vka);
            return error;
        }
        /* increment as we go to aid destruction */
        timer->to.nirqs++;
    }

    timer->to.nobjs = 0;
    return 0;
}

int sel4platsupport_init_default_timer_ops(vka_t *vka, UNUSED vspace_t *vspace, simple_t *simple, ps_io_ops_t ops,
                  seL4_CPtr ntfn, seL4_timer_t *timer)
{
    int error;
    if (timer == NULL) {
        return EINVAL;
    }

    error = ltimer_default_describe(&timer->ltimer, ops);

    if (!error) {
        size_t nirqs = get_nirqs(&timer->ltimer);
        for (size_t i = 0; i < nirqs; i++) {
            error = ltimer_get_nth_irq(&timer->ltimer, i, &timer->to.irqs[i].irq);
            if (error) {
                return error;
            }
        }
        error = init_timer_internal(vka, simple, ntfn, timer, nirqs);
    }

    if (!error)  {
        error = ltimer_default_init(&timer->ltimer, ops);
    }
    return error;
}

int sel4platsupport_init_default_timer(vka_t *vka, vspace_t *vspace, simple_t *simple,
                  seL4_CPtr ntfn, seL4_timer_t *timer)
{
    int error;

    /* initialise io ops */
    ps_io_ops_t ops;
    memset(&ops, 0, sizeof(ops));
    error = sel4platsupport_new_io_ops(*vspace, *vka, &ops);
    if (!error) {
        /* we have no way of storing the fact that we allocated these io ops so we'll just leak
         * them forever */
        return sel4platsupport_init_default_timer_ops(vka, vspace, simple, ops, ntfn, timer);
    }
    return error;
}

int sel4platsupport_init_timer_irqs(vka_t *vka, simple_t *simple,
                  seL4_CPtr ntfn, seL4_timer_t *timer, timer_objects_t *to)
{
    if (timer == NULL) {
        return EINVAL;
    }

    /* copy the timer objects */
    timer->to = *to;
    return init_timer_internal(vka, simple, ntfn, timer, to->nirqs);
}


int sel4platsupport_init_default_timer_caps(vka_t *vka, vspace_t *vspace, simple_t *simple, timer_objects_t *timer_objects)
{
    /* initialise io ops */
    ps_io_ops_t ops;
    memset(&ops, 0, sizeof(ops));
    int error = sel4platsupport_new_io_ops(*vspace, *vka, &ops);
    if (error) {
        ZF_LOGE("Failed to get io ops");
        return error;;
    }

    /* Allocate timer irqs. */
    ltimer_t ltimer;
    error = ltimer_default_describe(&ltimer, ops);
    if (error) {
        ZF_LOGE("Failed to describe default timer");
        return error;
    }

    /* set up the irq caps the timer needs */
    size_t nirqs = get_nirqs(&ltimer);
    for (size_t i = 0; i < nirqs; i++) {
        error = ltimer_get_nth_irq(&ltimer, i, &timer_objects->irqs[i].irq);
        assert(timer_objects->irqs[i].irq.type != PS_NONE);
        if (!error) {
            error =  sel4platsupport_copy_irq_cap(vka, simple, &timer_objects->irqs[i].irq,
                                          &timer_objects->irqs[i].handler_path);
        }
        if (error) {
            ZF_LOGE("Failed to get irq cap %zu", i);
            return error;
        }
        timer_objects->nirqs++;
    }

    /* Obtain untyped frame caps for PS default ltimer.
     * currently this code assumes all timers need a 4k frame.
     */
    size_t nobjs = ltimer_get_num_pmems(&ltimer);
    if (nobjs > MAX_OBJS) {
        ZF_LOGE("MAX_OBJS insufficient for timer");
        return -1;
    }

    for (size_t i = 0; i < nobjs; i++) {
        error = get_nth_pmem(vka, &ltimer, &timer_objects->objs[i], i);
        timer_objects->objs[i].obj.size_bits = seL4_PageBits;
        if (error) {
            ZF_LOGE("Failed to get nth pmem");
            return error;
        }
        /* increment as we go to aid destruction */
        timer_objects->nobjs++;
    }

    return error;
}

seL4_CPtr sel4platsupport_timer_objs_get_irq_cap(timer_objects_t *to, int id, irq_type_t type) {
    for (size_t i = 0; i < to->nirqs; i++) {
        if (to->irqs[i].irq.type == type) {
            switch (type) {
            case PS_MSI:
                if (to->irqs[i].irq.msi.vector == id) {
                    return to->irqs[i].handler_path.capPtr;
                }
                break;
            case PS_IOAPIC:
                if (to->irqs[i].irq.ioapic.pin == id) {
                    return to->irqs[i].handler_path.capPtr;
                }
                break;
            case PS_INTERRUPT:
                if (to->irqs[i].irq.irq.number == id) {
                    return to->irqs[i].handler_path.capPtr;
                }
                break;
            case PS_NONE:
                ZF_LOGE("Invalid irq type");
            }
        }
    }

    ZF_LOGE("Could not find irq");
    return seL4_CapNull;
}
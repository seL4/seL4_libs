/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#include <autoconf.h>
#include <sel4bench/flog.h>
#include <sel4utils/sched.h>
#include <sel4utils/sel4_zf_logif.h>
#include <string.h>
#include <utils/sglib.h>
#include <vka/capops.h>
#include <sel4/tfIPC.h>

/* define RB tree for EDF implementation */
typedef struct rb_tree {
    uint64_t weight;
    time_t budget;
    time_t period;
    cspacepath_t reply_path;
    cspacepath_t slot;
    seL4_CPtr tcb;
    seL4_CPtr sched_context;
    int id;
    bool reply_cap_saved;

    /* fields for sglib rb tree */
    char colour_field;
    struct rb_tree *left;
    struct rb_tree *right;
} edf_rb_tree_t;

static int
edf_compare(edf_rb_tree_t *x, edf_rb_tree_t *y)
{
    if (x->weight > y->weight) {
        return 1;
    } else if (x->weight < y->weight) {
        return -1;
    }
    return 0;
}

/* SGLIB emits some warnings when building, supress */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-variable"
SGLIB_DEFINE_RBTREE_PROTOTYPES(edf_rb_tree_t, left, right, colour_field, edf_compare);
SGLIB_DEFINE_RBTREE_FUNCTIONS(edf_rb_tree_t, left, right, colour_field, edf_compare);
#pragma GCC diagnostic pop

typedef struct edf_data {
    edf_rb_tree_t *release_tree;
    edf_rb_tree_t *deadline_tree;
    seL4_timer_t *clock_timer;
    seL4_timer_t *timeout_timer;
    cspacepath_t endpoint_path;
    vka_object_t endpoint;
    seL4_CPtr notification;
    int next_id;
    vka_t *vka;
    seL4_CPtr tcb;
} edf_data_t;

static edf_rb_tree_t *
head(edf_rb_tree_t *tree)
{
    struct sglib_edf_rb_tree_t_iterator it;
    return sglib_edf_rb_tree_t_it_init_inorder(&it, tree);
}

static void *
add_tcb(sched_t *sched, seL4_CPtr sched_context, void *params)
{
    edf_data_t *data = sched->data;
    struct edf_sched_add_tcb_args *args = (struct edf_sched_add_tcb_args *) params;
    int error;
    edf_rb_tree_t *node = calloc(1, sizeof(edf_rb_tree_t));
    if (node == NULL) {
        ZF_LOGE("Failed to allocate node\n");
        return NULL;
    }

    node->slot = args->slot;
    node->tcb = args->tcb;
    node->budget = args->budget;
    node->period = args->period;
    node->id = data->next_id;
    node->sched_context = sched_context;
    data->next_id++;
    node->weight = node->id;

    /* mint a copy of the endpoint cap into the tcb's cspace with a badge corresponding to the
     * scheduling ID of this thread */
    ZF_LOGD("Minting cap with badge %d\n", node->id);
    error = vka_cnode_mint(&args->slot, &data->endpoint_path, seL4_AllRights, seL4_CapData_Badge_new(node->id));
    if (error != seL4_NoError) {
        ZF_LOGE("Failed to mint cap to target slot: %d\n", error);
        free(node);
        return NULL;
    }

    /* create a slot in the schedulers cspace to save the reply cap in */
    ZF_LOGD("Creating reply cap path\n");
    error = vka_cspace_alloc_path(data->vka, &node->reply_path);
    if (error != seL4_NoError) {
        ZF_LOGE("Failed to alloc path for reply slot: %d\n", error);
        vka_cnode_delete(&args->slot);
        free(node);
        return NULL;
    }

    sglib_edf_rb_tree_t_add(&data->release_tree, node);
    assert(head(data->release_tree) != NULL);

    return (void *) node;
}

static uint64_t
release_threads(edf_rb_tree_t **release_tree, edf_rb_tree_t **deadline_tree, uint64_t time)
{
    edf_rb_tree_t *release_top = head(*release_tree);

    while (release_top != NULL && release_top->weight <= time) {
        ZF_LOGV("Released %d, time %llu, release time %llu\n", release_top->id, time, release_top->weight);

        /* remove from release tree */
        sglib_edf_rb_tree_t_delete(release_tree, release_top);

        /* update params for this job */
        release_top->weight = time + release_top->period;

        /* place in deadline tree */
        sglib_edf_rb_tree_t_add(deadline_tree, release_top);

        /* update release top */
        release_top = head(*release_tree);
    }

    /* return next release time */
    return release_top == NULL ? UINT64_MAX : release_top->weight;
}


static void
start(edf_data_t *data, uint64_t time)
{
    edf_rb_tree_t *current;

    current = head(data->release_tree);
    while (current != NULL) {
        assert(sglib_edf_rb_tree_t_is_member(data->release_tree, current));
        sglib_edf_rb_tree_t_delete(&data->release_tree, current);
        current->weight = time + current->period;
        sglib_edf_rb_tree_t_add(&data->deadline_tree, current);
        current = head(data->release_tree);
    }
}

static int
run_scheduler(sched_t *sched, bool (*finished)(void *cookie), void *cookie, void *args)
{
    edf_data_t *data = (edf_data_t *) sched->data;
    seL4_Word badge;
    seL4_MessageInfo_t info;
    int error;
    UNUSED flog_t *flog = (flog_t *) args;
    edf_rb_tree_t *prev = NULL;

    /* release all threads - scheduler starts now */
    start(data, timer_get_time(data->clock_timer->timer));
    timer_start(data->timeout_timer->timer);

    while (!finished(cookie)) {
        /* release threads */
        uint64_t time = timer_get_time(data->clock_timer->timer);
        uint64_t next_interrupt = release_threads(&data->release_tree, &data->deadline_tree, time);

        /* set timeout for next release */
        if (next_interrupt != UINT64_MAX) {
            error = timer_oneshot_relative(data->timeout_timer->timer, next_interrupt - time);
            if (unlikely(error == ETIME)) {
                error = timer_oneshot_relative(data->timeout_timer->timer, 4 * NS_IN_US);
                ZF_LOGF_IF(error, "Failed to set backup timer irq");
            }
            ZF_LOGF_IF(error, "Failed to set timeout for %llu - %llu = %llu, %d\n", next_interrupt,
                        time, next_interrupt - time, error);
        }

        /* schedule a thread */
        edf_rb_tree_t *current = head(data->deadline_tree);

        /* deal with previous thread */
        if (prev != NULL) {
            if (current == prev) {
                prev->reply_cap_saved = true;
            } else if (current) {
                /* install current's reply cap in to this threads TCB reply cap slot
                 * the previous threads reply cap (if it exists) is placed in currents->reply_path */
                ZF_LOGD("Swapped in current, out prev");
                error = vka_cnode_swapCaller(&current->reply_path);
                ZF_LOGF_IFERR(error, "Failed to swap caller");
                
                cspacepath_t tmp = current->reply_path;
                current->reply_path = prev->reply_path;
                prev->reply_path = tmp;
            } else if (!current) {
                /* just swap null in */
                ZF_LOGD("Swapped in null");
                error = vka_cnode_swapCaller(&prev->reply_path);
                ZF_LOGF_IFERR(error, "Failed to swap caller");
            }
            
            prev->reply_cap_saved = true;
        } else if (current && current->reply_cap_saved) {
            /* just swap null out */
            ZF_LOGD("Swapped in current");
            error = vka_cnode_swapCaller(&current->reply_path);
            ZF_LOGF_IFERR(error, "Failed to swap caller");
        }

        /* schedule current thread or set timer tick for next release */
        if (current != NULL) {
            if (!current->reply_cap_saved) {
                /* current was preempted */
                ZF_LOGD("Resuming job for thread %d\n", current->id);
                seL4_SchedContext_YieldTo_t result = seL4_SchedContext_YieldTo(current->sched_context);
                ZF_LOGF_IFERR(result.error, "YieldTo failed");
                flog_end(flog);
                info = seL4_Recv(data->endpoint_path.capPtr, &badge);
            } else {
                /* current is waiting for us to reply to it once its budget is refilled */
                ZF_LOGD("Releasing job for thread %d\n", current->id);
                current->reply_cap_saved = false;
                flog_end(flog);
                info = seL4_ReplyRecv(data->endpoint.cptr, info, &badge);
            }
       } else {
            /* noone to schedule */
            ZF_LOGD("Noone to schedule\n");
            flog_end(flog);
            info = seL4_Recv(data->endpoint.cptr, &badge);
        }

        flog_start(flog);
        ZF_LOGD("awake");

        /* wait for message or timer irq */
        switch (badge) {
        case 0:
            /* timer irq */
            ZF_LOGD("Tick\n");
            prev = NULL;
            sel4_timer_handle_single_irq(data->timeout_timer);
            break;
        default:
            ZF_LOGF_IF(current == NULL, "Got message when no thread scheduled!");
            ZF_LOGF_IF(current->id != badge, "Got message from wrong thread %d", badge);
            ZF_LOGD("Message from %d, fault? %d\n", badge, seL4_isTemporalFault_Tag(info));

            assert(sglib_edf_rb_tree_t_is_member(data->deadline_tree, current));
            sglib_edf_rb_tree_t_delete(&data->deadline_tree, current);
            sglib_edf_rb_tree_t_add(&data->release_tree, current);
            prev = current;
            break;
        }
    }

    timer_stop(data->timeout_timer->timer);
    return 0;
}

static void
destroy_tree(edf_data_t *data, edf_rb_tree_t **tree)
{
    edf_rb_tree_t *current;
    struct sglib_edf_rb_tree_t_iterator it;

    if (tree == NULL) {
        return;
    }

    current = sglib_edf_rb_tree_t_it_init(&it, *tree);
    while (current != NULL) {
        if (current->reply_cap_saved) {
            vka_cnode_delete(&current->reply_path);
        }
        vka_cspace_free(data->vka, current->reply_path.capPtr);
        vka_cnode_delete(&current->slot);
        free(current);
        current = sglib_edf_rb_tree_t_it_next(&it);

    }

    *tree = NULL;
}

static void
remove_tcb(sched_t *sched, void *tcb)
{
    edf_data_t *data = sched->data;

    if (sglib_edf_rb_tree_t_is_member(data->release_tree, tcb)) {
        sglib_edf_rb_tree_t_delete(&data->release_tree, tcb);
    } else if (sglib_edf_rb_tree_t_is_member(data->deadline_tree, tcb)) {
        sglib_edf_rb_tree_t_delete(&data->deadline_tree, tcb);
    } else {
        ZF_LOGW("Attempted to remove member from scheduler that was not present\n");
    }
}


static void
remove_all_tcbs(sched_t *sched)
{
    edf_data_t *data = sched->data;
    destroy_tree(data, &data->release_tree);
    destroy_tree(data, &data->deadline_tree);
    data->next_id = 1;
}

static void
destroy_scheduler(sched_t *sched)
{
    if (sched == NULL) {
        /* nothing to do */
        return;
    }

    edf_data_t *data = sched->data;

    /* unbind notification */
    if (data->tcb != seL4_CapNull) {
        seL4_TCB_UnbindNotification(data->tcb);
    }

    /* free created objects */
    if (data->vka != NULL) {
        if (data->endpoint.cptr != 0) {
            vka_free_object(data->vka, &data->endpoint);
        }
    }

    if (data != NULL) {
        destroy_tree(data, &data->release_tree);
        destroy_tree(data, &data->deadline_tree);
        free(data);
    }

    free(sched);
}

sched_t *
sched_new_edf(seL4_timer_t *clock_timer, seL4_timer_t *timeout_timer, vka_t *vka,
              seL4_CPtr tcb, seL4_CPtr notification)
{

    /* check clock timer has correct properties */
    if (!clock_timer->timer->properties.upcounter) {
        ZF_LOGE("Clock timer should be a 64 bit up counter\n");
        return NULL;
    }

    /* check timeout_timer has correct properties */
    if (!timeout_timer->timer->properties.timeouts) {
        ZF_LOGE("Timeout timer must support timeouts\n");
        return NULL;
    }

    /* create scheduler and scheduler data */
    sched_t *sched = (sched_t *) calloc(1, sizeof(sched_t));
    if (sched == NULL) {
        ZF_LOGE("Failed to allocatec sched\n");
        return NULL;
    }

    edf_data_t *edf_data = (edf_data_t *) calloc(1, sizeof(struct edf_data));
    if (edf_data == NULL) {
        ZF_LOGE("Failed to allocate edf_data\n");
        destroy_scheduler(sched);
        return NULL;
    }

    /* create an IPC endpoint for EDF threads to IPC on to signal job completion */
    if (vka_alloc_endpoint(vka, &edf_data->endpoint)) {
        ZF_LOGE("Failed to allocate endpoint\n");
        destroy_scheduler(sched);
        return NULL;
    }
    vka_cspace_make_path(vka, edf_data->endpoint.cptr, &edf_data->endpoint_path);
    /* bind the notificatino to the scheduler TCB, such that it can listen for notifications
     * and IPCs on the same endpoint */
    if (seL4_TCB_BindNotification(tcb, notification) != seL4_NoError) {
        ZF_LOGE("Failed to bind tcb %x to notification %x\n", tcb, notification);
        destroy_scheduler(sched);
        return NULL;
    }
    edf_data->tcb = tcb;

    sched->add_tcb = add_tcb;
    sched->run_scheduler = run_scheduler;
    sched->destroy_scheduler = destroy_scheduler;
    sched->remove_tcb = remove_tcb;
    sched->reset = remove_all_tcbs;

    edf_data->next_id = 1;
    sched->data = edf_data;
    edf_data->clock_timer = clock_timer;
    edf_data->timeout_timer = timeout_timer;
    edf_data->vka = vka;

    /* create deadline tree */
    edf_data->deadline_tree = NULL;
    edf_data->release_tree = NULL;

    return sched;
}




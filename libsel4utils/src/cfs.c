/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#include <utils/sglib.h>
#include <sel4bench/flog.h>
#include <sel4utils/sched.h>
#include <vka/capops.h>
#include <vka/vka.h>
#include <string.h>

/* define RB tree for scheduler implementation */
typedef struct rb_tree {
    uint64_t weight;
    union {
        /* non preemptive */
        seL4_CPtr sched_context;
        struct {
            cspacepath_t reply_path;
        };
    };
    int id;

    /* fields for sglib rb tree */
    char colour_field;
    struct rb_tree *left;
    struct rb_tree *right;
} cfs_rb_tree_t;

static int
cfs_compare(cfs_rb_tree_t *x, cfs_rb_tree_t *y)
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
SGLIB_DEFINE_RBTREE_PROTOTYPES(cfs_rb_tree_t, left, right, colour_field, cfs_compare);
SGLIB_DEFINE_RBTREE_FUNCTIONS(cfs_rb_tree_t, left, right, colour_field, cfs_compare);
#pragma GCC diagnostic pop

typedef struct cfs_data {
    cfs_rb_tree_t *tree;
    int next_id;
    vka_t *vka;
    seL4_CPtr sched_context;
    vka_object_t endpoint;
} cfs_data_t;

static cfs_rb_tree_t *
head(cfs_rb_tree_t *tree)
{
    struct sglib_cfs_rb_tree_t_iterator it;
    return sglib_cfs_rb_tree_t_it_init_inorder(&it, tree);
}

static void
remove_from_tree(sched_t *sched, void *tcb)
{
    cfs_data_t *data = sched->data;
    if (tcb && sglib_cfs_rb_tree_t_is_member(data->tree, tcb)) {
        sglib_cfs_rb_tree_t_delete(&data->tree, tcb);
    } else {
        ZF_LOGW("Attempted to remove member from scheduler that was not present\n");
    }
}

static void
preempt_remove_tcb(sched_t *sched, void *tcb)
{
    remove_from_tree(sched, tcb);

    if (tcb) {
        free(tcb);
    }
}

static void
coop_remove_tcb(sched_t *sched, void *tcb)
{
    cfs_data_t *data = sched->data;
    cfs_rb_tree_t *node = tcb;

    remove_from_tree(sched, tcb);

    if (node && node->reply_path.capPtr != 0) {
        vka_cnode_delete(&node->reply_path);
        vka_cspace_free(data->vka, node->reply_path.capPtr);
    }

    if (tcb) {
        free(node);
    }
}

static void *
preempt_add_tcb(sched_t *sched, seL4_CPtr sched_context, void *params)
{
    cfs_data_t *data = sched->data;
    cfs_rb_tree_t *node = calloc(1, sizeof(cfs_rb_tree_t));

    if (node == NULL) {
        ZF_LOGE("Failed to allocate node\n");
        return NULL;
    }

    node->sched_context = sched_context;

    sglib_cfs_rb_tree_t_add(&data->tree, node);
    assert(head(data->tree) != NULL);

    return node;
}

static void *
coop_add_tcb(sched_t *sched, seL4_CPtr sched_context, void *params)
{
    cfs_data_t *data = sched->data;
    cfs_rb_tree_t *node = preempt_add_tcb(sched, sched_context, params);
    struct cfs_sched_add_tcb_args *args = params;

    if (node == NULL) {
        return NULL;
    }

    /* allocate cslot for reply cap to be saved in  */
    if (vka_cspace_alloc_path(data->vka, &node->reply_path) != 0) {
        coop_remove_tcb(sched, node);
        return NULL;
    }

    /* copy ep cap into provided slot */
    cspacepath_t ep_path;
    vka_cspace_make_path(data->vka, data->endpoint.cptr, &ep_path);
    if (vka_cnode_copy(&args->slot, &ep_path, seL4_AllRights) != seL4_NoError) {
        coop_remove_tcb(sched, node);
        return NULL;
    }

    ZF_LOGD("Wait for first message from client\n");
    /* wait for first message from client */
    seL4_Recv(data->endpoint.cptr, NULL);

    /* convert to passive */
    ZF_LOGD("Convert to passive\n");
    if (seL4_SchedContext_Unbind(sched_context) != seL4_NoError) {
        ZF_LOGE("Failed to unbind scheduling context");
        coop_remove_tcb(sched, node);
        return NULL;
    }

     /* save reply cap */
    ZF_LOGD("Save reply cap\n");
    if (vka_cnode_swapCaller(&node->reply_path) != seL4_NoError) {
        ZF_LOGE("Failed to save caller");
        coop_remove_tcb(sched, node);
        return NULL;
    }

    return node;
}

static cfs_rb_tree_t *
pick(cfs_data_t *data)
{
    cfs_rb_tree_t *current = head(data->tree);
    sglib_cfs_rb_tree_t_delete(&data->tree, current);
    ZF_LOGD("Picked %p\n", current);
    return current;
}

static void
add(cfs_data_t *data, cfs_rb_tree_t *current, seL4_Time consumed)
{
    /* put the node back in the heap */
    current->weight += consumed;
    sglib_cfs_rb_tree_t_add(&data->tree, current);
}

static int
coop_run_scheduler(sched_t *sched, bool (*finished)(void *cookie), void *cookie, void *args)
{

    cfs_data_t *data = sched->data;
    cfs_rb_tree_t *prev = head(data->tree);
    UNUSED int error;
    seL4_SchedContext_Consumed_t consumed;
    UNUSED flog_t *flog = (flog_t *) args;

    if (head(data->tree) == NULL) {
        ZF_LOGE("noone to schedule");
        return -1;
    }

    flog_start(flog);
    cfs_rb_tree_t *current = pick(data);
    error = vka_cnode_swapCaller(&current->reply_path);

    while (!finished(cookie)) {

        /* swap in currents reply cap */
        if (current != prev) {
            error = vka_cnode_swapCaller(&current->reply_path);
            assert(error == seL4_NoError);

            /* swap paths */
            cspacepath_swap(&current->reply_path, &prev->reply_path);
        }

        /* wait for current to yield */
        ZF_LOGV("Waiting for thread\n");
        flog_end(flog);
        seL4_ReplyRecv(data->endpoint.cptr, seL4_MessageInfo_new(0, 0, 0, 1), NULL);
        flog_start(flog);

        consumed = seL4_SchedContext_Consumed(data->sched_context);
        add(data, current, consumed.consumed);

        prev = current;
        current = pick(data);
    }

    return 0;
}

static int
preempt_run_scheduler(sched_t *sched, bool (*finished)(void *cookie), void *cookie, void *args)
{

    cfs_data_t *data = (cfs_data_t *) sched->data;
    seL4_SchedContext_YieldTo_t consumed;
    UNUSED flog_t *flog = (flog_t *) args;

    if (head(data->tree) == NULL) {
        ZF_LOGE("noone to schedule");
        return -1;
    }

    flog_start(flog);
    while (!finished(cookie)) {
        /* schedule a thread */
        cfs_rb_tree_t *current = pick(data);

        /* wait for callers timeslice to expire */
        flog_end(flog);
        consumed = seL4_SchedContext_YieldTo(current->sched_context);
        flog_start(flog);
        ZF_LOGD("%p ran for %llu\n", current, consumed.consumed);

        assert(consumed.error == seL4_NoError);

        /* put the node back in the heap */
        add(data, current, consumed.consumed);
    }

    return 0;
}

static void
destroy_tree(sched_t *sched)
{
    cfs_data_t *data = sched->data;

    if (data->tree == NULL) {
        return;
    }

    cfs_rb_tree_t *current = head(data->tree);
    while (current != NULL) {
        sched_remove_tcb(sched, current);
        current = head(data->tree);
    }

    data->tree = NULL;
}

static void
reset(sched_t *sched)
{
    destroy_tree(sched);
}

static void
preempt_destroy_scheduler(sched_t *sched)
{
    if (sched == NULL) {
        /* nothing to do */
        return;
    }

    cfs_data_t *data = sched->data;

    if (data != NULL) {
        destroy_tree(sched);
        free(data);
    }

    free(sched);
}

static void
coop_destroy_scheduler(sched_t *sched)
{
    if (sched != NULL && sched->data != NULL) {
        cfs_data_t *data = (cfs_data_t *) sched->data;
        if (data->endpoint.cptr != seL4_CapNull) {
            vka_free_object(data->vka, &data->endpoint);
        }

        preempt_destroy_scheduler(sched);
    }
}

static sched_t *
sched_new_cfs_common(vka_t *vka)
{
    /* create scheduler and scheduler data */
    sched_t *sched = calloc(1, sizeof(sched_t));
    if (sched == NULL) {
        ZF_LOGE("Failed to allocate sched\n");
        return NULL;
    }

    cfs_data_t *cfs_data = calloc(1, sizeof(struct cfs_data));
    if (cfs_data == NULL) {
        ZF_LOGE("Failed to allocate cfs_data\n");
        preempt_destroy_scheduler(sched);
        return NULL;
    }

    cfs_data->vka = vka;
    sched->reset = reset;
    cfs_data->tree = NULL;
    sched->data = cfs_data;

    return sched;
}

sched_t *
sched_new_preemptive_cfs(void)
{
    sched_t *sched = sched_new_cfs_common(NULL);
    if (sched == NULL) {
        return NULL;
    }

    sched->add_tcb = preempt_add_tcb;
    sched->remove_tcb = preempt_remove_tcb;
    sched->run_scheduler = preempt_run_scheduler;
    sched->destroy_scheduler = preempt_destroy_scheduler;

    return sched;
}

sched_t *
sched_new_cooperative_cfs(vka_t *vka, seL4_CPtr sched_context)
{
    sched_t *sched = sched_new_cfs_common(vka);
    if (sched == NULL) {
        return NULL;
    }
    cfs_data_t *data = sched->data;

    if (vka_alloc_endpoint(vka, &data->endpoint)) {
        ZF_LOGE("Failed to allocate endpoint\n");
        preempt_destroy_scheduler(sched);
        return NULL;
    }

    data->sched_context = sched_context;
    sched->add_tcb = coop_add_tcb;
    sched->run_scheduler = coop_run_scheduler;
    sched->destroy_scheduler = coop_destroy_scheduler;
    sched->remove_tcb = coop_remove_tcb;

    return sched;
}


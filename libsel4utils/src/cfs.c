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
#include <sel4utils/sched.h>
#include <string.h>

/* define RB tree for scheduler implementation */
typedef struct rb_tree {
    uint64_t weight;
    seL4_CPtr sched_context;
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
    seL4_CPtr tcb;
} cfs_data_t;

static cfs_rb_tree_t *
head(cfs_rb_tree_t *tree)
{
    struct sglib_cfs_rb_tree_t_iterator it;
    return sglib_cfs_rb_tree_t_it_init_inorder(&it, tree);
}

static void *
add_tcb(sched_t *sched, seL4_CPtr sched_context, void *params)
{
    cfs_data_t *data = sched->data;
    cfs_rb_tree_t *node = malloc(sizeof(cfs_rb_tree_t));
    if (node == NULL) {
        ZF_LOGE("Failed to allocate node\n");
        return NULL;
    }

    node->weight = 0;
    node->sched_context = sched_context;

    sglib_cfs_rb_tree_t_add(&data->tree, node);
    assert(head(data->tree) != NULL);

    return node;
}


static void
remove_tcb(sched_t *sched, void *tcb)
{
    cfs_data_t *data = sched->data;

    if (sglib_cfs_rb_tree_t_is_member(data->tree, tcb)) {
        sglib_cfs_rb_tree_t_delete(&data->tree, tcb);
    } else {
        ZF_LOGW("Attempted to remove member from scheduler that was not present\n");
    }
}

static int
run_scheduler(sched_t *sched, bool (*finished)(void *cookie), void *cookie)
{

    cfs_data_t *data = (cfs_data_t *) sched->data;

    while (!finished(cookie)) {
        /* schedule a thread */
        cfs_rb_tree_t *current = head(data->tree);
        sglib_cfs_rb_tree_t_delete(&data->tree, current);
        ZF_LOGD("Picked %p\n", current);
        while (current == NULL && !finished(cookie)) {
            ZF_LOGE("No threads to schedule\n");
            return -1;
        }

        seL4_SchedContext_YieldTo_t consumed = {0};
        consumed = seL4_SchedContext_YieldTo(current->sched_context);
        ZF_LOGD("%p ran for %llu\n", current, consumed.consumed);

        /* we won't return until that thread runs */
        if (consumed.error != seL4_NoError) {
            /* unless there was an error */
            ZF_LOGE("Error: YieldTo failed\n");
            return -1;
        }

        /* put the node back in the heap */
        current->weight += consumed.consumed;
        sglib_cfs_rb_tree_t_add(&data->tree, current);
    }

    return 0;
}

static void
destroy_tree(cfs_rb_tree_t **tree)
{
    cfs_rb_tree_t *current;
    struct sglib_cfs_rb_tree_t_iterator it;

    if (tree == NULL) {
        return;
    }

    current = sglib_cfs_rb_tree_t_it_init(&it, *tree);
    while (current != NULL) {
        free(current);
        current = sglib_cfs_rb_tree_t_it_next(&it);
    }

    *tree = NULL;
}

static void
reset(sched_t *sched)
{
    cfs_data_t *data = sched->data;
    destroy_tree(&data->tree);
}


static void
destroy_scheduler(sched_t *sched)
{
    if (sched == NULL) {
        /* nothing to do */
        return;
    }

    cfs_data_t *data = sched->data;

    if (data != NULL) {
        destroy_tree(&data->tree);
        free(data);
    }

    free(sched);
}

sched_t *
sched_new_cfs(void)
{
    /* create scheduler and scheduler data */
    sched_t *sched = (sched_t *) malloc(sizeof(sched_t));
    if (sched == NULL) {
        ZF_LOGE("Failed to allocate sched\n");
        return NULL;
    }

    cfs_data_t *cfs_data = (cfs_data_t *) malloc(sizeof(struct cfs_data));
    if (cfs_data == NULL) {
        ZF_LOGE("Failed to allocate cfs_data\n");
        destroy_scheduler(sched);
        return NULL;
    }
    memset(cfs_data, 0, sizeof(struct cfs_data));

    sched->add_tcb = add_tcb;
    sched->run_scheduler = run_scheduler;
    sched->destroy_scheduler = destroy_scheduler;
    sched->reset = reset;
    sched->remove_tcb = remove_tcb;
    sched->data = cfs_data;
    cfs_data->tree = NULL;

    return sched;
}


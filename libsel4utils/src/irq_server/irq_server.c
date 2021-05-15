/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sel4utils/irq_server.h>

#include <errno.h>
#include <simple/simple.h>
#include <sel4utils/thread.h>
#include <vka/capops.h>
#include <stdlib.h>
#include <string.h>
#include <platsupport/irq.h>
#include <sel4platsupport/irq.h>

#include <utils/util.h>

#define IRQ_SERVER_MESSAGE_LENGTH 2

typedef struct irq_server_node {
    seL4_CPtr ntfn;
    size_t max_irqs_bound;
    size_t num_irqs_bound;
} irq_server_node_t;

typedef struct irq_server_thread irq_server_thread_t;

/* This is also forwarded declared as we have a pointer to a struct of the same type */
struct irq_server_thread {
    thread_id_t thread_id;
    irq_server_node_t *node;
    seL4_CPtr delivery_ep;
    seL4_Word label;
    sel4utils_thread_t thread;
    /* Linked list chain of threads */
    irq_server_thread_t *next;
};

/* This is forward-declared in the header file, hence no typedef */
struct irq_server {
    seL4_CPtr delivery_ep;
    seL4_Word label;
    vka_object_t reply;
    irq_server_thread_t *server_threads;
    size_t num_irqs;
    size_t max_irqs;

    /* New thread parameters */
    seL4_Word priority;
    seL4_CPtr cspace;

    /* Allocation interfaces */
    vka_t *vka;
    vspace_t *vspace;
    simple_t *simple;
    ps_irq_ops_t irq_ops;
    ps_malloc_ops_t *malloc_ops;
};

/* Executes the registered callback for incoming IRQs */
static void irq_server_node_handle_irq(irq_server_thread_t *thread_info,
                                       ps_irq_ops_t *irq_ops, seL4_Word badge)
{
    ntfn_id_t target_ntfn = thread_info->thread_id;
    int error = sel4platsupport_irq_handle(irq_ops, target_ntfn, badge);
    if (error) {
        if (error == -EINVAL) {
            ZF_LOGE("Passed in a wrong ntfn_id to the IRQ interface! Something is very wrong with the IRQ server");
        } else {
            ZF_LOGW("Failed to handle an IRQ");
        }
    }
}

/* Registers an IRQ callback and enables the IRQ */
static irq_id_t irq_server_node_register_irq(irq_server_node_t *node, ps_irq_t irq, irq_callback_fn_t callback,
                                             void *callback_data, ntfn_id_t ntfn_id, irq_server_t *irq_server)
{
    int error;

    irq_id_t irq_id = ps_irq_register(&(irq_server->irq_ops), irq, callback, callback_data);
    if (irq_id < 0) {
        ZF_LOGE("Failed to register an IRQ");
        /* The ID also serves as an error code */
        return irq_id;
    }

    error = sel4platsupport_irq_set_ntfn(&(irq_server->irq_ops), ntfn_id, irq_id, NULL);
    if (error) {
        ZF_LOGE("Failed to pair an IRQ with a notification");
        ps_irq_unregister(&(irq_server->irq_ops), irq_id);
        return error;
    }

    node->num_irqs_bound++;

    /* Success, return the ID that was assigned to the IRQ */
    return irq_id;
}

/* Creates a new IRQ server node */
static irq_server_node_t *irq_server_node_new(seL4_CPtr ntfn, size_t max_irqs_bound,
                                              ps_malloc_ops_t *malloc_ops)
{
    irq_server_node_t *new_node = NULL;
    ps_calloc(malloc_ops, 1, sizeof(irq_server_node_t), (void **) &new_node);
    if (new_node) {
        new_node->ntfn = ntfn;
        new_node->max_irqs_bound = max_irqs_bound;
    }
    return new_node;
}

/* IRQ handler thread. Wait on a notification object for IRQs. When one arrives, send a
 * synchronous message to the registered endpoint. If no synchronous endpoint was
 * registered, call the appropriate handler function directly (must be thread safe) */
static void _irq_thread_entry(irq_server_thread_t *my_thread_info, ps_irq_ops_t *irq_ops)
{
    seL4_CPtr ep;
    seL4_CPtr ntfn;
    uintptr_t thread_info_ptr;
    seL4_Word label;

    ep = my_thread_info->delivery_ep;
    ntfn = my_thread_info->node->ntfn;
    thread_info_ptr = (uintptr_t)my_thread_info;
    label = my_thread_info->label;
    ZF_LOGD("thread started. Waiting on endpoint %lu\n", ntfn);

    while (1) {
        seL4_Word badge = 0;
        seL4_Wait(ntfn, &badge);
        if (ep != seL4_CapNull) {
            /* Synchronous endpoint registered. Send IPC */
            seL4_MessageInfo_t info = seL4_MessageInfo_new(label, 0, 0, IRQ_SERVER_MESSAGE_LENGTH);
            seL4_SetMR(0, badge);
            seL4_SetMR(1, thread_info_ptr);
            seL4_Send(ep, info);
        } else {
            /* No synchronous endpoint. Get the IRQ interface to invoke callbacks */
            irq_server_node_handle_irq(my_thread_info, irq_ops, badge);
        }
    }
}

thread_id_t irq_server_thread_new(irq_server_t *irq_server, seL4_CPtr provided_ntfn,
                                  seL4_Word usable_mask, thread_id_t id_hint)
{
    int error;

    irq_server_thread_t *new_thread = NULL;

    irq_server_node_t *new_node = NULL;

    /* Check if the user provided a notification, if not, then allocate one */
    seL4_CPtr ntfn_to_use = seL4_CapNull;
    seL4_Word mask_to_use = 0;
    vka_object_t ntfn_obj = {0};
    if (provided_ntfn != seL4_CapNull) {
        ntfn_to_use = provided_ntfn;
        if (usable_mask == 0) {
            ZF_LOGE("Can't pair any interrupts on this notification");
            return -EINVAL;
        }
        mask_to_use = usable_mask;
    } else {
        error = vka_alloc_notification(irq_server->vka, &ntfn_obj);
        if (error) {
            ZF_LOGE("Failed to allocate a notification");
            return -ENOMEM;
        }
        ntfn_to_use = ntfn_obj.cptr;
        mask_to_use = MASK(seL4_BadgeBits);
    }

    thread_id_t thread_id_to_use = -1;
    /* Register this notification with the IRQ interface */
    if (id_hint >= 0) {
        error = sel4platsupport_irq_provide_ntfn_with_id(&(irq_server->irq_ops), ntfn_to_use,
                                                         mask_to_use, id_hint);
        if (error) {
            goto fail;
        }
        thread_id_to_use = id_hint;
    } else {
        ntfn_id_t ntfn_id = sel4platsupport_irq_provide_ntfn(&(irq_server->irq_ops), ntfn_to_use, mask_to_use);
        if (ntfn_id < 0) {
            /* 'sel4platsupport_irq_provide_ntfn' returns an error code on failure */
            error = ntfn_id;
            goto fail;
        }
        thread_id_to_use = ntfn_id;
    }

    error = ps_calloc(irq_server->malloc_ops, 1, sizeof(irq_server_thread_t), (void **) &new_thread);
    if (new_thread == NULL) {
        ZF_LOGE("Failed to allocate memory for bookkeeping structure");
        /* ps_calloc returns errnos but these are greater than 0 */
        error *= -1;
        goto fail;
    }

    /* Find the amount of IRQs that can be bound to this node */
    size_t max_irqs_bound = POPCOUNTL(mask_to_use);
    /* Allocate memory for the node */
    new_node = irq_server_node_new(ntfn_to_use, max_irqs_bound, irq_server->malloc_ops);
    if (new_node == NULL) {
        error = -ENOMEM;
        goto fail;
    }

    /* Initialise structure */
    new_thread->delivery_ep = irq_server->delivery_ep;
    new_thread->label = irq_server->label;
    new_thread->node = new_node;
    new_thread->thread_id = thread_id_to_use;

    /* Create the IRQ thread */
    sel4utils_thread_config_t config = thread_config_default(irq_server->simple, irq_server->cspace,
                                                             seL4_NilData, 0, irq_server->priority);
    error = sel4utils_configure_thread_config(irq_server->vka, irq_server->vspace,
                                              irq_server->vspace, config, &(new_thread->thread));
    if (error) {
        ZF_LOGE("Failed to configure IRQ server thread");
        goto fail;
    }

    bool thread_created = true;

    /* Start the thread */
    error = sel4utils_start_thread(&new_thread->thread, (void *)_irq_thread_entry,
                                   new_thread, &(irq_server->irq_ops), 1);
    if (error) {
        ZF_LOGE("Failed to start IRQ server thread");
        goto fail;
    }

    /* Append this thread structure to the head of the list */
    new_thread->next = irq_server->server_threads;
    irq_server->server_threads = new_thread;

    return thread_id_to_use;

fail:

    /* Check if we've registered a notification with the IRQ interface */
    if (thread_id_to_use) {
        ZF_LOGF_IF(sel4platsupport_irq_return_ntfn(&(irq_server->irq_ops), thread_id_to_use, NULL),
                   "Failed to clean-up a failure situation");
    }

    if (ntfn_obj.cptr) {
        vka_free_object(irq_server->vka, &ntfn_obj);
    }

    if (new_node) {
        ps_free(irq_server->malloc_ops, sizeof(irq_server_node_t), new_node);
    }

    if (thread_created) {
        sel4utils_clean_up_thread(irq_server->vka, irq_server->vspace, &(new_thread->thread));
    }

    if (new_thread) {
        ps_free(irq_server->malloc_ops, sizeof(irq_server_thread_t), new_node);
    }

    return error;
}

void irq_server_handle_irq_ipc(irq_server_t *irq_server, seL4_MessageInfo_t msginfo)
{
    seL4_Word badge = 0;
    uintptr_t thread_info_ptr = 0;

    seL4_Word label = seL4_MessageInfo_get_label(msginfo);
    seL4_Word length = seL4_MessageInfo_get_length(msginfo);

    if (label == irq_server->label && length == IRQ_SERVER_MESSAGE_LENGTH) {
        badge = seL4_GetMR(0);
        thread_info_ptr = seL4_GetMR(1);
        if (thread_info_ptr == 0) {
            ZF_LOGE("Invalid data in IRQ server IPC\n");
        } else {
            irq_server_node_handle_irq((irq_server_thread_t *) thread_info_ptr, &(irq_server->irq_ops), badge);
        }
    }
}

/* Register for a function to be called when an IRQ arrives */
irq_id_t irq_server_register_irq(irq_server_t *irq_server, ps_irq_t irq,
                                 irq_callback_fn_t callback, void *callback_data)
{
    if (irq_server == NULL) {
        ZF_LOGE("irq_server is NULL");
        return -EINVAL;
    }

    irq_server_thread_t *st = NULL;
    irq_id_t ret_id = 0;

    /* Try to assign the IRQ to an existing node/thread */
    for (st = irq_server->server_threads; st != NULL; st = st->next) {
        if (st->node->num_irqs_bound < st->node->max_irqs_bound) {
            /* thread_id is synonymous with a ntfn_id */
            ret_id = irq_server_node_register_irq(st->node, irq, callback, callback_data,
                                                  (ntfn_id_t) st->thread_id, irq_server);
            if (ret_id >= 0) {
                return ret_id;
            }
        }
    }

    /* No threads are available to take this IRQ, alert the user */
    ZF_LOGE("No threads are available to take this interrupt, consider making more");
    return -ENOENT;
}

irq_server_t *irq_server_new(vspace_t *vspace, vka_t *vka, seL4_Word priority,
                             simple_t *simple, seL4_CPtr cspace, seL4_CPtr delivery_ep, seL4_Word label,
                             size_t num_irqs, ps_malloc_ops_t *malloc_ops)
{
    if (num_irqs < 0) {
        ZF_LOGE("num_irqs is less than 0");
        return NULL;
    }

    if (!vspace || !vka || !simple || !malloc_ops) {
        ZF_LOGE("ret_irq_server is NULL");
        return NULL;
    }

    int error;

    irq_server_t *new = NULL;

    error = ps_calloc(malloc_ops, 1, sizeof(irq_server_t), (void **) &new);

    if (config_set(CONFIG_KERNEL_MCS) && vka_alloc_reply(vka, &(new->reply)) != 0) {
        ZF_LOGE("Failed to allocate reply object");
        return NULL;
    }

    /* Set max_ntfn_ids to equal the number of IRQs. We can calculate the ntfn IDs we need,
     * but this is really complex, and leads to code that is hard to maintain. */
    irq_interface_config_t irq_config = { .max_irq_ids = num_irqs, .max_ntfn_ids = num_irqs } ;
    error = sel4platsupport_new_irq_ops(&(new->irq_ops), vka, simple, irq_config, malloc_ops);
    if (error) {
        ZF_LOGE("Failed to initialise supporting backend for IRQ server");
        return NULL;
    }

    new->delivery_ep = delivery_ep;
    new->label = label;
    new->max_irqs = num_irqs;
    new->priority = priority;

    new->vka = vka;
    new->vspace = vspace;
    new->simple = simple;
    new->cspace = cspace;
    new->malloc_ops = malloc_ops;

    return new;
}

seL4_MessageInfo_t irq_server_wait_for_irq(irq_server_t *irq_server, seL4_Word *ret_badge)
{
    seL4_MessageInfo_t msginfo = {0};
    seL4_Word badge = 0;

    if (irq_server->delivery_ep == seL4_CapNull) {
        ZF_LOGE("No endpoint was registered with the IRQ server");
        return msginfo;
    }

    /* Wait for an event, api_recv instead of seL4_Recv b/c of MCS */
    msginfo = api_recv(irq_server->delivery_ep, &badge, irq_server->reply.cptr);
    if (ret_badge) {
        *ret_badge = badge;
    }

    /* Forward to IRQ handlers */
    irq_server_handle_irq_ipc(irq_server, msginfo);

    return msginfo;
}

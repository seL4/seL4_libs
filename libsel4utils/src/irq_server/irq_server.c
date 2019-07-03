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

/* Executes the registered callback for incoming IRQS */
static void
irq_server_node_handle_irq(struct irq_server_node *n, seL4_Word badge)
{
    struct irq_data* irqs;
    irqs = n->irqs;
    /* Mask out reserved bits */
    badge = badge & n->badge_mask;
    /* For each bit, call the registered handler */
    while (badge) {
        int irq_idx;
        struct irq_data* irq;
        irq_idx = CTZL(badge);
        irq = &irqs[irq_idx];
        ZF_LOGD("Received IRQ %d, badge 0x%x, index %d\n", irq->irq, (unsigned)badge, irq_idx);
        irq->cb(irq);
        badge &= ~BIT(irq_idx);
    }
}


/* Registers an IRQ callback and enabled the IRQ */
struct irq_data*
irq_server_node_register_irq(irq_server_node_t n, irq_t irq, irq_handler_fn cb,
                             void* token, vka_t* vka, seL4_CPtr cspace,
                             simple_t *simple) {
    struct irq_data* irqs;
    int i;
    irqs = n->irqs;

    for (i = 0; i < NIRQS_PER_NODE; i++) {
        /* If a cap has not been registered and the bit in the mask is not set */
        if (irqs[i].cap == seL4_CapNull && (n->badge_mask & BIT(i))) {
            irqs[i].cap = irq_bind(irq, n->notification, i, vka, simple);
            if (irqs[i].cap == seL4_CapNull) {
                ZF_LOGD("Failed to bind IRQ\n");
                return NULL;
            }
            irqs[i].irq = irq;
            irqs[i].cb = cb;
            irqs[i].token = token;
            return &irqs[i];
        }
    }
    return NULL;
}

/* Creates a new IRQ server node which contains Thread data and registered IRQ data. */
struct irq_server_node*
irq_server_node_new(seL4_CPtr notification, seL4_Word badge_mask) {
    struct irq_server_node *n;
    n = calloc(1, sizeof(*n));
    if (n) {
        n->notification = notification;
        n->badge_mask = badge_mask;
    }
    return n;
}

/* IRQ handler thread. Wait on a notification object for IRQs. When one arrives, send a
 * synchronous message to the registered endpoint. If no synchronous endpoint was
 * registered, call the appropriate handler function directly (must be thread safe) */
static void
_irq_thread_entry(struct irq_server_thread* st)
{
    seL4_CPtr sep;
    seL4_CPtr notification;
    uintptr_t node_ptr;
    seL4_Word label;

    sep = st->delivery_sep;
    notification = st->node->notification;
    node_ptr = (uintptr_t)st->node;
    label = st->label;
    ZF_LOGD("thread started. Waiting on endpoint %d\n", notification);

    while (1) {
        seL4_Word badge;
        seL4_Wait(notification, &badge);
        assert(badge != 0);
        if (sep != seL4_CapNull) {
            /* Synchronous endpoint registered. Send IPC */
            seL4_MessageInfo_t info = seL4_MessageInfo_new(label, 0, 0, 2);
            seL4_SetMR(0, badge);
            seL4_SetMR(1, node_ptr);
            seL4_Send(sep, info);
        } else {
            /* No synchronous endpoint. Call the handler directly */
            irq_server_node_handle_irq(st->node, badge);
        }
    }
}

/* Creates a new thread for an IRQ server */
struct irq_server_thread*
irq_server_thread_new(vspace_t* vspace, vka_t* vka, seL4_CPtr cspace, seL4_Word priority,
                      simple_t *simple, seL4_Word label, seL4_CPtr sep) {
    struct irq_server_thread* st;
    int err;

    /* Allocate memory for the structure */
    st = malloc(sizeof(*st));
    if (st == NULL) {
        return NULL;
    }
    st->node = irq_server_node_new(0, MASK(NIRQS_PER_NODE));
    if (st->node == NULL) {
        free(st);
        return NULL;
    }

    /* Initialise structure */
    st->delivery_sep = sep;
    st->label = label;
    st->next = NULL;
    /* Create an endpoint to listen on */
    err = vka_alloc_notification(vka, &st->notification);
    if (err) {
        ZF_LOGE("Failed to allocate IRQ notification endpoint for IRQ server thread\n");
        return NULL;
    }
    st->node->notification = st->notification.cptr;
    /* Create the IRQ thread */
    sel4utils_thread_config_t config = thread_config_default(simple, cspace, seL4_NilData, 0, priority);
    err = sel4utils_configure_thread_config(vka, vspace, vspace, config, &st->thread);

    if (err) {
        ZF_LOGE("Failed to configure IRQ server thread\n");
        return NULL;
    }
    /* Start the thread */
    err = sel4utils_start_thread(&st->thread, (void*)_irq_thread_entry, st, NULL, 1);
    if (err) {
        ZF_LOGE("Failed to start IRQ server thread\n");
        return NULL;
    }
    return st;
}

/******************
 *** IRQ server ***
 ******************/


/* Handle an incoming IPC from a server node */
void
irq_server_handle_irq_ipc(irq_server_t irq_server UNUSED)
{
    seL4_Word badge;
    uintptr_t node_ptr;

    badge = seL4_GetMR(0);
    node_ptr = seL4_GetMR(1);
    if (node_ptr == 0) {
        ZF_LOGE("Invalid data in irq server IPC\n");
    } else {
        irq_server_node_handle_irq((struct irq_server_node*)node_ptr, badge);
    }
}

/* Register for a function to be called when an IRQ arrives */
struct irq_data*
irq_server_register_irq(irq_server_t irq_server, irq_t irq,
                        irq_handler_fn cb, void* token) {
    struct irq_server_thread* st;
    struct irq_data* irq_data;

    /* Try to assign the IRQ to an existing node */
    for (st = irq_server->server_threads; st != NULL; st = st->next) {
        irq_data = irq_server_node_register_irq(st->node, irq, cb, token,
                                                irq_server->vka, irq_server->cspace,
                                                &irq_server->simple);
        if (irq_data) {
            return irq_data;
        }
    }
    /* Try to create a new node */
    if (st == NULL && irq_server->max_irqs < 0) {
        /* Create the node */
        ZF_LOGD("Spawning new IRQ server thread\n");
        st = irq_server_thread_new(irq_server->vspace, irq_server->vka, irq_server->cspace,
                                   irq_server->thread_priority, &irq_server->simple,
                                   irq_server->label, irq_server->delivery_ep);
        if (st == NULL) {
            ZF_LOGE("Failed to create server thread\n");
            return NULL;
        }

        st->next = irq_server->server_threads;
        irq_server->server_threads = st;
        irq_data = irq_server_node_register_irq(st->node, irq, cb, token,
                                                irq_server->vka, irq_server->cspace,
                                                &irq_server->simple);
        if (irq_data) {
            return irq_data;
        }
    }
    /* Give up */
    ZF_LOGD("Failed to register for IRQ %d\n", irq);
    return NULL;
}

/* Create a new IRQ server */
int
irq_server_new(vspace_t* vspace, vka_t* vka, seL4_CPtr cspace, seL4_Word priority,
               simple_t *simple, seL4_CPtr sync_ep, seL4_Word label,
               int nirqs, irq_server_t *ret_irq_server)
{
    struct irq_server* irq_server;

    /* Structure allocation and initialisation */
    irq_server = malloc(sizeof(*irq_server));
    if (irq_server == NULL) {
        ZF_LOGE("malloc failed on irq server memory allocation");
        return -1;
    }

    if (config_set(CONFIG_KERNEL_RT) && vka_alloc_reply(vka, &irq_server->reply) != 0) {
        ZF_LOGE("Failed to allocate reply object");
        free(irq_server);
        return -1;
    }

    irq_server->delivery_ep = sync_ep;
    irq_server->label = label;
    irq_server->max_irqs = nirqs;
    irq_server->vspace = vspace;
    irq_server->cspace = cspace;
    irq_server->vka = vka;
    irq_server->thread_priority = priority;
    irq_server->server_threads = NULL;
    irq_server->simple = *simple;

    /* If a fixed number of IRQs are requested, create and start the server threads */
    if (nirqs > -1) {
        struct irq_server_thread** server_thread;
        int n_nodes;
        int i;
        server_thread = &irq_server->server_threads;
        n_nodes = (nirqs + NIRQS_PER_NODE - 1) / NIRQS_PER_NODE;
        for (i = 0; i < n_nodes; i++) {
            *server_thread = irq_server_thread_new(vspace, vka, cspace, priority,
                                                   simple, label, sync_ep);
            server_thread = &(*server_thread)->next;
        }
    }

    *ret_irq_server = irq_server;
    return 0;
}

seL4_MessageInfo_t
irq_server_wait_for_irq(irq_server_t irq_server, seL4_Word* badge_ret)
{
    seL4_MessageInfo_t msginfo;
    seL4_Word badge;

    /* Wait for an event */
    msginfo = api_recv(irq_server->delivery_ep, &badge, irq_server->reply.cptr);
    if (badge_ret) {
        *badge_ret = badge;
    }

    /* Forward to IRQ handlers */
    if (seL4_MessageInfo_get_label(msginfo) == irq_server->label) {
        irq_server_handle_irq_ipc(irq_server);
    }
    return msginfo;
}

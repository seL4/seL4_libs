/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * The IRQ server helps to manage the IRQs in a system. There are 2 API levels
 * in the design:
 *   1. [irq server thread] An IRQ server node is a standalone thread which waits
 *                          for the arrival of any IRQ that is being managed by a
 *                          particular irq server node. When an IRQ is received,
 *                          the irq server thread will either forward the irq
 *                          information to a registered synchronous endpoint, or
 *                          call the appropriate handler function directly.
 *   2. [irq server]        A dynamic collection of server threads. When
 *                          registering an IRQ call back function, the irq server
 *                          will attempt to deligate the IRQ to an irq server node
 *                          that has not yet reached capacity. If no node can accept
 *                          the IRQ, it will alert the user that a new irq server
 *                          thread needs to be created to support the additional demand.
 *
 * ++ Special notes ++
 *
 * Performance
 *   The application can achieve greater performance by configuring the irq
 *   server, or irq server threads, to call the IRQ handler functions directly.
 *   This is done by not providing an endpoint in the creation of an irq server
 *   interface. The application must take care to ensure that all concurrency
 *   issues are addressed.
 *
 * Resource availability
 *   The irq server API family accept resource allocators as arguments to some
 *   function calls. These resource allocators
 *   must be kept available indefinitely.
 */

#pragma once

#include <autoconf.h>
#include <sel4utils/gen_config.h>

#include <sel4/sel4.h>
#include <vspace/vspace.h>
#include <vka/vka.h>
#include <simple/simple.h>
#include <platsupport/io.h>
#include <platsupport/irq.h>
#include <sel4platsupport/irq.h>

typedef struct irq_server irq_server_t;

typedef int thread_id_t;

/**
 * Initialises an IRQ server. The server will manage threads that are
 * explicitly created by the user to handle incoming IRQs. The server
 * delegates most of the IRQ cap creation and pairing to the IRQ interface in
 * libsel4platsupport.
 * @param[in] vspace            The current vspace
 * @param[in] vka               Allocator for creating kernel objects
 * @param[in] priority          The priority of spawned threads
 * @param[in] simple            A simple interface for creating IRQ caps
 * @param[in] cspace            The cspace of the current thread
 * @param[in] delivery_ep       The endpoint to send alerts about IRQ's to
 * @param[in] label             A label to use when sending to an IPC to an endpoint
 * @param[in] nirqs             The maximum number of irqs to support
 * @return                      A valid pointer to an irq_server_t instance, otherwise NULL
 */
irq_server_t *irq_server_new(vspace_t *vspace, vka_t *vka, seL4_Word priority,
                             simple_t *simple, seL4_CPtr cspace, seL4_CPtr delivery_ep,
                             seL4_Word label, size_t num_irqs, ps_malloc_ops_t *malloc_ops);

/**
 * Creates a new thread to wait on IRQs.
 * Upon receiving a signal that an IRQ has arrived, the thread will send an IPC message
 * through to the endpoint that was registered with the IRQ server, if any. Otherwise,
 * it will call 'irq_server_handle_irq_ipc' to handle the interrupt.
 *
 * The user can provide a notification for the thread to use, and can mark which bits
 * of the badge that the thread can use for IRQs. If a null cap was provided,
 * the IRQ server can create a notification cap on behalf of the user, the 'usable_mask'
 * parameter will be ignored in this case.
 *
 * A ID will be assigned to this thread and
 * returned to the user, but the user can provide a 'hint' to IRQ server. The server will
 * attempt to assign an ID with the same value as the hint.
 * @param[in] irq_server        A handle to the IRQ server
 * @param[in] provided_ntfn     Notification cap to be provided to thread, can be
 *                              'seL4_CapNull'
 * @param[in] usable_mask       Mask of bits indicating which bits of the badge space
 *                              the thread can use, will be ignore if 'provided_ntfn' is
 *                              'null'
 * @param[in] id_hint           'Hint' to be passed to the IRQ server when assigning an
 *                              ID, >= 0 for a valid hint, -1 otherwise
 */
thread_id_t irq_server_thread_new(irq_server_t *irq_server, seL4_CPtr provided_ntfn,
                                  seL4_Word usable_mask, thread_id_t id_hint);

/**
 * Enable an IRQ and register a callback function. This functionality is
 * delegated to the IRQ interface in libplatsupport.
 * @param[in] irq_server        The IRQ server which shall be responsible for the IRQ
 * @param[in] irq               Information about the IRQ that will be registered
 * @param[in] callback          A callback function to call when the requested IRQ arrives
 * @param[in] callback_data     Client data which should be passed to the registered call
 *                              back function
 * @return                      On success, returns an ID for the IRQ that was assigned by
 *                              the IRQ interface in libsel4platsuport. Otherwise, returns
 *                              an error code
 */
irq_id_t irq_server_register_irq(irq_server_t *irq_server, ps_irq_t irq,
                                 irq_callback_fn_t callback, void *callback_data);

/**
 * Redirects control to the IRQ subsystem to process an arriving IRQ.  The
 * server will read the appropriate message registers to retrieve the
 * information that it needs. This should be called only when an IPC message
 * with the designated label was received on an endpoint that was given to an
 * IRQ server instance.
 * @param[in] irq_server   The IRQ server which is responsible for the received IRQ
 * @param[in] msginfo      An IPC message info header of an IPC message received on
 *                         an endpoint registered with an IRQ server
 */
void irq_server_handle_irq_ipc(irq_server_t *irq_server, seL4_MessageInfo_t msginfo);

/**
 * Waits on the IRQ delivery endpoint for the next IRQ. If an IPC is received, but the
 * label does not match that which was assigned to the IRQ server, the message info and
 * badge are returned to the caller, much like seL4_Recv(...). If the label does match,
 * irq_server_handle_irq_ipc(...) will be called before returning.
 *
 * If an endpoint was not registered with the IRQ server that is passed in, this function
 * is essentially a no-op.
 * @param[in]  irq_server  A handle to the IRQ server
 * @param[out] ret_badge   If badge_ret is not NULL, the received badge will be
 *                         written to badge_ret.
 * @return                 The seL4_MessageInfo_t structure as provided by the
 *                         seL4 kernel in response to a seL4_Recv call. The
 *                         caller may check that the label matches that which
 *                         was registered for the IRQ server in order to
 *                         determine if the received event was destined for the
 *                         IRQ server or if the thread was activated by some
 *                         other IPC event.
 *                         On the error case where the IRQ server passed
 *                         in does not have an endpoint passed in, an empty
 *                         seL4_MessageInfo_t struct will be returned.
 */
seL4_MessageInfo_t irq_server_wait_for_irq(irq_server_t *irq_server, seL4_Word *ret_badge);

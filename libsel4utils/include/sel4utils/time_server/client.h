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
#pragma once

#include <platsupport/ltimer.h>

/* the timer op is set in mr0 */
typedef enum rpc_timer_ops {
    /* get the time. Return error code in mr0, and time values in mr1, mr2 (only use mr2 on 32-bit) */
    GET_TIME = 1,
    /* set a timeout. mr1 is the timeout type, mr2 (+mr3 on 32bit) store the timeout value. */
    SET_TIMEOUT = 2
} rpc_ltimer_ops_t;

/**
 * Initialise a client ltimer which calls via rpc to a server for
 * timer operations. This exists such that it is easy to swap out ltimer
 * hardware implementations and rpc implementations.
 *
 * This provides the RPC stubs for requesting a timeout and getting the time, how they are
 * implemented depends on the server/client policy. While ltimer_handle_irq can be called for
 * this timer it is not required and has no effect.
 *
 * Users of this interface must provide a mechanism for waiting for a timeout to come in, as per
 * usual for ltimer interfaces.
 *
 * @param ltimer interface to initialise
 * @param ps_io_ops to allocate memory with
 * @param ep the endpoint to client should RPC for timer operations
 * @param label the label to use in timer RPC messages, so they can be identified on the server side.
 * @return 0 on success
 */
int sel4utils_rpc_ltimer_init(ltimer_t *ltimer, ps_io_ops_t ops,
                              seL4_CPtr ep, seL4_Word label);


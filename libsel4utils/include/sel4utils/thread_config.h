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
/**
 *
 * Builder functions for thread configs
 *
 */

#include <autoconf.h>
#include <sel4/types.h>
#include <simple/simple.h>

/* Threads and processes use this struct as both need scheduling config parameters */
typedef struct sched_params {
     /* seL4 priority for the thread to be scheduled with. */
    uint8_t priority;
    /* seL4 maximum controlled priority for the thread. */
    uint8_t mcp;
    /* TCB to derive MCP from when setting priority/mcp (in future API) */
    seL4_CPtr auth;
} sched_params_t;

typedef struct sel4utils_thread_config {
    /* fault_endpoint endpoint to set as the threads fault endpoint. Can be seL4_CapNull. */
    seL4_CPtr fault_endpoint;
   /* root of the cspace to start the thread in */
    seL4_CNode cspace;
    /* data for cspace access */
    seL4_CapData_t cspace_root_data;
    /* use a custom stack size? */
    bool custom_stack_size;
    /* custom stack size in 4k pages for this thread */
    seL4_Word stack_size;
    /* true if this thread should have no ipc buffer */
    bool no_ipc_buffer;
    /* scheduling parameters */
    sched_params_t sched_params;
} sel4utils_thread_config_t;

static inline sel4utils_thread_config_t
thread_config_cspace(sel4utils_thread_config_t config, seL4_CPtr cspace_root, seL4_CapData_t cspace_root_data)
{
    config.cspace = cspace_root;
    config.cspace_root_data = cspace_root_data;
    return config;
}

static inline sel4utils_thread_config_t
thread_config_auth(sel4utils_thread_config_t config, seL4_CPtr tcb)
{
    config.sched_params.auth = tcb;
    return config;
}

static inline sel4utils_thread_config_t
thread_config_new(simple_t *simple)
{
    sel4utils_thread_config_t config = {0};
    seL4_CapData_t data = seL4_CapData_Guard_new(0, seL4_WordBits - simple_get_cnode_size_bits(simple));
    config = thread_config_auth(config, simple_get_tcb(simple));
    return thread_config_cspace(config, simple_get_cnode(simple), data);
}

static inline sel4utils_thread_config_t
thread_config_priority(sel4utils_thread_config_t config, uint8_t priority)
{
    config.sched_params.priority = priority;
    return config;
}

static inline sel4utils_thread_config_t
thread_config_mcp(sel4utils_thread_config_t config, uint8_t mcp)
{
    config.sched_params.mcp = mcp;
    return config;
}

static inline sel4utils_thread_config_t
thread_config_stack_size(sel4utils_thread_config_t config, seL4_Word size)
{
    config.stack_size = size;
    config.custom_stack_size = true;
    return config;
}

static inline sel4utils_thread_config_t
thread_config_no_ipc_buffer(sel4utils_thread_config_t config)
{
    config.no_ipc_buffer = true;
    return config;
}

static inline sel4utils_thread_config_t
thread_config_fault_endpoint(sel4utils_thread_config_t config, seL4_CPtr fault_ep)
{
    config.fault_endpoint = fault_ep;
    return config;
}

static inline sel4utils_thread_config_t
thread_config_default(simple_t *simple, seL4_CPtr cnode, seL4_CapData_t data, seL4_CPtr fault_ep, uint8_t prio)
{
    sel4utils_thread_config_t config = thread_config_new(simple);
    config = thread_config_cspace(config, cnode, data);
    config = thread_config_fault_endpoint(config, fault_ep);
    config = thread_config_priority(config, prio);
    return config;
}

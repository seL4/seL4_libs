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
#include <sel4utils/api.h>

/* Threads and processes use this struct as both need scheduling config parameters */
typedef struct sched_params {
     /* seL4 priority for the thread to be scheduled with. */
    uint8_t priority;
    /* seL4 maximum controlled priority for the thread. */
    uint8_t mcp;
    /* TCB to derive MCP from when setting priority/mcp (in future API) */
    seL4_CPtr auth;
    /* true if sel4utils should create an sc */
    bool create_sc;
    /* sel4 sched control cap for creating sc */
    seL4_CPtr sched_ctrl;
    /* scheduling parameters */
    uint64_t period;
    uint64_t budget;
    seL4_Word extra_refills;
    seL4_Word badge;
    /* otherwise use this provided sc */
    seL4_CPtr sched_context;
    /* affinity for this tcb. Must set sched_ctrl if CONFIG_RT */
    seL4_Word core;
} sched_params_t;

typedef struct sel4utils_thread_config {
    /* fault_endpoint endpoint to set as the threads fault endpoint. Can be seL4_CapNull. */
    seL4_CPtr fault_endpoint;
   /* root of the cspace to start the thread in */
    seL4_CNode cspace;
    /* data for cspace access */
    seL4_Word cspace_root_data;
    /* use a custom stack size? */
    bool custom_stack_size;
    /* custom stack size in 4k pages for this thread */
    seL4_Word stack_size;
    /* true if this thread should have no ipc buffer */
    bool no_ipc_buffer;
    /* scheduling parameters */
    sched_params_t sched_params;
    /* true if sel4utils should create a reply */
    bool create_reply;
    /* otherwise provide one */
    seL4_CPtr reply;
} sel4utils_thread_config_t;

static inline sched_params_t
sched_params_periodic(sched_params_t params, simple_t *simple, seL4_Word core, uint64_t period_us,
                      uint64_t budget_us, seL4_Word extra_refills, seL4_Word badge)
{
    if (!config_set(CONFIG_KERNEL_RT)) {
        ZF_LOGW("Setting sched params on non-RT kernel will have no effect");
    }
    params.sched_ctrl = simple_get_sched_ctrl(simple, core);
    params.period = period_us;
    params.budget = budget_us;
    params.extra_refills = extra_refills;
    params.badge = badge;
    params.create_sc = true;
    return params;
}

static inline sched_params_t
sched_params_round_robin(sched_params_t params, simple_t *simple, seL4_Word core, uint64_t timeslice_us)
{
    return sched_params_periodic(params, simple, core, timeslice_us, timeslice_us, 0, 0);
}

static inline sched_params_t
sched_params_core(sched_params_t params, seL4_Word core)
{
    if (!config_set(CONFIG_KERNEL_RT)) {
        ZF_LOGW("Setting core on RT kernel will have no effect - sched ctrl required");
    }
    params.core = core;
    return params;
}

static inline sel4utils_thread_config_t
thread_config_create_reply(sel4utils_thread_config_t config)
{
    config.create_reply = true;
    return config;
}

static inline sel4utils_thread_config_t
thread_config_reply(sel4utils_thread_config_t config, seL4_CPtr reply)
{
    config.create_reply = false;
    config.reply = reply;
    return config;
}

static inline sel4utils_thread_config_t
thread_config_sched_context(sel4utils_thread_config_t config, seL4_CPtr sched_context)
{
    config.sched_params.create_sc = false;
    config.sched_params.sched_context = sched_context;
    return config;
}

static inline sel4utils_thread_config_t
thread_config_cspace(sel4utils_thread_config_t config, seL4_CPtr cspace_root, seL4_Word cspace_root_data)
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
    seL4_Word data = api_make_guard_skip_word(seL4_WordBits - simple_get_cnode_size_bits(simple));
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
thread_config_default(simple_t *simple, seL4_CPtr cnode, seL4_Word data, seL4_CPtr fault_ep, uint8_t prio)
{
    sel4utils_thread_config_t config = thread_config_new(simple);
    config = thread_config_cspace(config, cnode, data);
    config = thread_config_fault_endpoint(config, fault_ep);
    config = thread_config_priority(config, prio);
    if (config_set(CONFIG_KERNEL_RT)) {
        uint64_t timeslice = CONFIG_BOOT_THREAD_TIME_SLICE;
        config.sched_params = sched_params_round_robin(config.sched_params, simple, 0, timeslice * US_IN_MS);
    }
    config = thread_config_create_reply(config);
    return config;
}

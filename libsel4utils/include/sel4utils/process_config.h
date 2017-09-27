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
/* Builder functions for process configs */
#include <autoconf.h>
#include <sel4/types.h>
#include <sel4utils/api.h>
#include <sel4utils/elf.h>
#include <vka/vka.h>

typedef struct {
    /* should we handle elf logic at all? */
    bool is_elf;
    /* if so what is the image name? */
    const char *image_name;
    /* Do you want the elf image preloaded? */
    bool do_elf_load;

    /* otherwise what is the entry point and sysinfo? */
    void *entry_point;
    uintptr_t sysinfo;

    /* should we create a default single level cspace? */
    bool create_cspace;
    /* if so how big ? */
    int one_level_cspace_size_bits;
    /* if not by what CPtr can the initial thread of the process
     * invoke its own TCB (optional) */
    seL4_CPtr dest_cspace_tcb_cptr;

    /* otherwise what is the root cnode ?*/
    /* Note if you use a custom cspace then
     * sel4utils_copy_cap_to_process etc will not work */
    vka_object_t cnode;

    /* do you want us to create a vspace for you? */
    bool create_vspace;
    /* if not what is the page dir, and what is the vspace */
    vspace_t *vspace;
    vka_object_t page_dir;

    /* if so, is there a regions you want left clear?*/
    sel4utils_elf_region_t *reservations;
    int num_reservations;

    /* do you want a fault endpoint created? */
    bool create_fault_endpoint;
    /* otherwise what is it */
    vka_object_t fault_endpoint;

    /* scheduling params */
    sched_params_t sched_params;

    seL4_CPtr asid_pool;
} sel4utils_process_config_t;

static inline sel4utils_process_config_t
process_config_asid_pool(sel4utils_process_config_t config, seL4_CPtr asid_pool)
{
    config.asid_pool = asid_pool;
    return config;
}

static inline sel4utils_process_config_t
process_config_auth(sel4utils_process_config_t config, seL4_CPtr auth)
{
    config.sched_params.auth = auth;
    return config;
}

static inline sel4utils_process_config_t
process_config_new(simple_t *simple)
{
    sel4utils_process_config_t config = {0};
    config = process_config_auth(config, simple_get_tcb(simple));
    return process_config_asid_pool(config, simple_get_init_cap(simple, seL4_CapInitThreadASIDPool));
}

static inline sel4utils_process_config_t
process_config_elf(sel4utils_process_config_t config, const char *image_name, bool preload)
{
    config.is_elf = true;
    config.image_name = image_name;
    config.do_elf_load = preload;
    return config;
}

static inline sel4utils_process_config_t
process_config_noelf(sel4utils_process_config_t config, void *entry_point, uintptr_t sysinfo)
{
    config.is_elf = false;
    config.entry_point = entry_point;
    config.sysinfo = sysinfo;
    return config;
}

static inline sel4utils_process_config_t
process_config_cnode(sel4utils_process_config_t config, vka_object_t cnode)
{
    config.create_cspace = false;
    config.cnode = cnode;
    return config;
}

static inline sel4utils_process_config_t
process_config_create_cnode(sel4utils_process_config_t config, int size_bits)
{
    config.create_cspace = true;
    config.one_level_cspace_size_bits = size_bits;
    return config;
}

static inline sel4utils_process_config_t
process_config_vspace(sel4utils_process_config_t config, vspace_t *vspace, vka_object_t page_dir)
{
    config.create_vspace = false;
    config.vspace = vspace;
    config.page_dir = page_dir;
    return config;
}

static inline sel4utils_process_config_t
process_config_create_vspace(sel4utils_process_config_t config, sel4utils_elf_region_t *reservations,
                             int num_reservations)
{
    config.create_vspace = true;
    config.reservations = reservations;
    config.num_reservations = num_reservations;
    return config;
}

static inline sel4utils_process_config_t
process_config_priority(sel4utils_process_config_t config, uint8_t priority)
{
    config.sched_params.priority = priority;
    return config;
}

static inline sel4utils_process_config_t
process_config_mcp(sel4utils_process_config_t config, uint8_t mcp)
{
    config.sched_params.mcp = mcp;
    return config;
}

static inline sel4utils_process_config_t
process_config_create_fault_endpoint(sel4utils_process_config_t config)
{
    config.create_fault_endpoint = true;
    return config;
}

static inline sel4utils_process_config_t
process_config_fault_endpoint(sel4utils_process_config_t config, vka_object_t fault_endpoint)
{
    config.fault_endpoint = fault_endpoint;
    config.create_fault_endpoint = false;
    return config;
}

static inline sel4utils_process_config_t
process_config_fault_cptr(sel4utils_process_config_t config, seL4_CPtr fault_cptr)
{
    config.fault_endpoint.cptr = fault_cptr;
    config.create_fault_endpoint = false;
    return config;
}

static inline sel4utils_process_config_t
process_config_default(const char *image_name, seL4_CPtr asid_pool)
{
    sel4utils_process_config_t config = {0};
    config = process_config_asid_pool(config, asid_pool);
    config = process_config_elf(config, image_name, true);
    config = process_config_create_cnode(config, CONFIG_SEL4UTILS_CSPACE_SIZE_BITS);
    config = process_config_create_vspace(config, NULL, 0);
    return process_config_create_fault_endpoint(config);
}

static inline sel4utils_process_config_t
process_config_default_simple(simple_t *simple, const char *image_name, uint8_t prio)
{
    sel4utils_process_config_t config = process_config_new(simple);
    config = process_config_elf(config, image_name, true);
    config = process_config_create_cnode(config, CONFIG_SEL4UTILS_CSPACE_SIZE_BITS);
    config = process_config_create_vspace(config, NULL, 0);
    config = process_config_create_fault_endpoint(config);

    if (config_set(CONFIG_KERNEL_RT)) {
        uint64_t timeslice = CONFIG_BOOT_THREAD_TIME_SLICE;
        config.sched_params = sched_params_round_robin(config.sched_params, simple, 0, timeslice * US_IN_MS);
    }

    return process_config_priority(config, prio);
}

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

#include <sel4bench/types.h>
#include <sel4bench/cpu/private.h>
#include <assert.h>
#include <sel4/sel4.h>

//begin system-mode stubs for sel4bench_* api functions
static KERNELFN void sel4bench_private_init(void* data)
{
    sel4bench_arm1136_pmnc_t init_pmnc = {
        {
            .C = 1, //reset CCNT
            .P = 1, //reset performance counters
            .Flag = 0x7, //reset overflow flags
            .E = 1, //start all counters
        }
    };

    //reset and enable all counters
    sel4bench_private_set_pmnc(init_pmnc);
}

static KERNELFN void sel4bench_private_destroy(void* data)
{
    sel4bench_arm1136_pmnc_t kill_pmnc = {
        {
            .C = 1, //reset CCNT
            .P = 1, //reset performance counters
            .Flag = 0x7, //reset overflow flags
            .E = 0, //stop all counters
        }
    };

    //disable and reset all counters
    sel4bench_private_set_pmnc(kill_pmnc);
}

static KERNELFN void sel4bench_private_reset_gp_counters(void* data)
{
    sel4bench_arm1136_pmnc_t reset_pmnc = sel4bench_private_get_pmnc();
    reset_pmnc.P = 1; //reset performance counters
    reset_pmnc.Flag = 0x3; //clear overflow flags on C0 and C1
    reset_pmnc.C = 0; //don't touch CCNT
    reset_pmnc.SBZ1 = 0; //per spec
    reset_pmnc.SBZ2 = 0; //per spec

    //reset and enable all counters
    sel4bench_private_set_pmnc(reset_pmnc);
}

static KERNELFN void sel4bench_private_get_cycle_count(void* data)
{
    //get contents of PMNC
    sel4bench_arm1136_pmnc_t pmnc_contents = sel4bench_private_get_pmnc();
    pmnc_contents.Flag = 0; //don't reset anything
    pmnc_contents.SBZ1 = 0; //per spec
    pmnc_contents.SBZ2 = 0; //per spec

    //store current state
    sel4bench_arm1136_pmnc_t pmnc_contents_restore = pmnc_contents;

    //disable counters
    pmnc_contents.E = 0;
    sel4bench_private_set_pmnc(pmnc_contents);

    //read value of specified counter
    *(uint32_t*)data = sel4bench_private_get_ccnt();

    //restore previous state
    sel4bench_private_set_pmnc(pmnc_contents_restore);
}

static KERNELFN void sel4bench_private_get_counter(void* data)
{
    //get contents of PMNC
    sel4bench_arm1136_pmnc_t pmnc_contents = sel4bench_private_get_pmnc();
    pmnc_contents.Flag = 0; //don't reset anything
    pmnc_contents.SBZ1 = 0; //per spec
    pmnc_contents.SBZ2 = 0; //per spec

    //store current state
    sel4bench_arm1136_pmnc_t pmnc_contents_restore = pmnc_contents;

    //disable counters
    pmnc_contents.E = 0;
    sel4bench_private_set_pmnc(pmnc_contents);

    //read value of specified counter
    *(uint32_t*)data = *(uint32_t*)data ? sel4bench_private_get_pmn1() : sel4bench_private_get_pmn0();

    //restore previous state
    sel4bench_private_set_pmnc(pmnc_contents_restore);
}

static KERNELFN void sel4bench_private_get_counters(void* data)
{
    //get contents of PMNC
    sel4bench_arm1136_pmnc_t pmnc_contents = sel4bench_private_get_pmnc();
    pmnc_contents.Flag = 0; //don't reset anything
    pmnc_contents.SBZ1 = 0; //per spec
    pmnc_contents.SBZ2 = 0; //per spec

    //store current state
    sel4bench_arm1136_pmnc_t pmnc_contents_restore = pmnc_contents;

    //disable counters
    pmnc_contents.E = 0;
    sel4bench_private_set_pmnc(pmnc_contents);

    uint32_t* args = (uint32_t*)data;
    uint32_t mask = args[0];
    ccnt_t* values = (ccnt_t*)args[1];

    //read value of specified counters
    if (mask & 1) {
        values[0] = sel4bench_private_get_pmn0();
    }
    if (mask & 2) {
        values[1] = sel4bench_private_get_pmn1();
    }

    //read CCNT
    args[0] = sel4bench_private_get_ccnt();

    //restore previous state
    sel4bench_private_set_pmnc(pmnc_contents_restore);
}

static KERNELFN void sel4bench_private_set_count_event(void* data)
{
    //bring in arguments
    uint32_t counter = ((uint32_t)data) >> 31;
    uint32_t event = ((uint32_t)data) & ~(1U << 31);

    //get contents of PMNC
    sel4bench_arm1136_pmnc_t pmnc_contents = sel4bench_private_get_pmnc();
    pmnc_contents.Flag = 0; //don't reset anything
    pmnc_contents.SBZ1 = 0; //per spec
    pmnc_contents.SBZ2 = 0; //per spec

    //store current state
    sel4bench_arm1136_pmnc_t pmnc_contents_restore = pmnc_contents;

    //stop counters
    pmnc_contents.E = 0;
    sel4bench_private_set_pmnc(pmnc_contents);

    //setup specified counter
    if (counter) {
        //setting counter 1
        pmnc_contents_restore.EvtCount2 = event;
        sel4bench_private_set_pmn1(0);
    } else {
        //setting counter 0
        pmnc_contents_restore.EvtCount1 = event;
        sel4bench_private_set_pmn0(0);

    }

    //restore previous state
    sel4bench_private_set_pmnc(pmnc_contents_restore);
}

/* Silence warnings about including the following functions when seL4_DebugRun
 * is not enabled when we are not calling them. If we actually call these
 * functions without seL4_DebugRun enabled, we'll get a link failure, so this
 * should be OK.
 */
void seL4_DebugRun(void (* userfn) (void *), void* userarg);

//begin actual sel4bench_* api functions

static FASTFN void sel4bench_init()
{
    seL4_DebugRun(&sel4bench_private_init, NULL);
}

static FASTFN void sel4bench_destroy()
{
    seL4_DebugRun(&sel4bench_private_destroy, NULL);
}

static FASTFN seL4_Word sel4bench_get_num_counters()
{
    return SEL4BENCH_ARM1136_NUM_COUNTERS;
}

static FASTFN ccnt_t sel4bench_get_cycle_count()
{
    uint32_t val = 0;
    seL4_DebugRun(&sel4bench_private_get_cycle_count, &val);
    return val;
}

static FASTFN ccnt_t sel4bench_get_counter(counter_t counter)
{
    assert(counter < sel4bench_get_num_counters()); //range check

    uint32_t val = counter;
    seL4_DebugRun(&sel4bench_private_get_counter, &val);
    return val;
}

static FASTFN ccnt_t sel4bench_get_counters(counter_bitfield_t mask, ccnt_t* values)
{
    assert(mask & (sel4bench_get_num_counters() - 1)); //there are only two counters, so there should be no other 1 bits
    assert(values);                                        //NULL guard -- because otherwise we'll get a kernel fault

    uint32_t args[2] = {mask, (uint32_t)values};
    //entry 0: in = mask, out = ccnt
    //entry 1: in = values
    seL4_DebugRun(&sel4bench_private_get_counters, args);
    return args[0];
}

static FASTFN void sel4bench_set_count_event(counter_t counter, event_id_t event)
{
    assert(counter < sel4bench_get_num_counters()); //range check

    seL4_DebugRun(&sel4bench_private_set_count_event, (void*)(event | ((counter & 1U) << 31)));
}

static FASTFN void sel4bench_stop_counters(counter_bitfield_t mask)
{
    /* all three ARM1136 counters have to start at the same time...
     * so we just start them all at init time and make this a no-op
     */
}

static FASTFN void sel4bench_reset_counters(void)
{
    seL4_DebugRun(&sel4bench_private_reset_gp_counters, NULL);
}

static FASTFN void sel4bench_start_counters(counter_bitfield_t mask)
{
    /* all three ARM1136 counters have to start at the same time...
     * so we just start them all at init time and make this reset them
     */
    sel4bench_reset_counters();
}

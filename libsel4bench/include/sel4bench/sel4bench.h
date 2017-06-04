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

#include <stddef.h>
#include <stdint.h>

#include <sel4/sel4.h>
#include <sel4bench/types.h>
#include <sel4bench/arch/sel4bench.h>
#include <utils/attribute.h>
#include <utils/arith.h>

/**
 * @file
 *
 * libsel4bench is a library designed to abstract over the performance
 * monitoring counters (PMCs) in modern IA-32 and ARM processors, so that you
 * can measure the performance of your software.  It will also work out whether
 * certain operations need to be done in kernel mode, and perform kernel code
 * injection calls to make them happen. As a result, expect that any library
 * call could potentially result in a syscall. (This is of particular note on
 * the ARM1136, for which even reading the cycle counter must be done in kernel
 * mode.)
 *
 * It also goes out of its way to ensure that there's always a cycle counter
 * available for use. _init() will start this running, and _destroy() will tear
 * it down, if necessary.
 *
 * Notes:
 * - Overflow is completely ignored, even on processors that only support
 *   32-bit counters (and thus where there is space to overflow into). If you
 *   are doing something that might overflow a counter, it's up to you to deal
 *   with that possibility.
 * - Everything is zero-indexed.
 */

/*
 * CPP constants for events that are common to all architecture
 * variants.
 * Additional events are architecture (and potentially processor) specific.
 * These may be defined in architecture or processor header files.
 */
static UNUSED event_id_t GENERIC_EVENTS[] = {
    SEL4BENCH_EVENT_CACHE_L1I_MISS,
    SEL4BENCH_EVENT_CACHE_L1D_MISS,
    SEL4BENCH_EVENT_TLB_L1I_MISS,
    SEL4BENCH_EVENT_TLB_L1D_MISS,
    SEL4BENCH_EVENT_EXECUTE_INSTRUCTION,
    SEL4BENCH_EVENT_BRANCH_MISPREDICT,
    SEL4BENCH_EVENT_MEMORY_ACCESS,
};

static UNUSED char * GENERIC_EVENT_NAMES[] = {
    "L1 i-cache misses",
    "L1 d-cache misses",
    "L1 i-tlb misses",
    "L1 d-tlb misses",
    "Instructions",
    "Branch mispredict",
    "Memory access",
};

static_assert(ARRAY_SIZE(GENERIC_EVENTS) == ARRAY_SIZE(GENERIC_EVENT_NAMES),
              "event names same length as counters");

/* Number of generic counters */
#define SEL4BENCH_NUM_GENERIC_EVENTS ARRAY_SIZE(GENERIC_EVENTS)

/**
 * Initialise the sel4bench library. Nothing else is guaranteed to work, and
 * may produce strange failures, if you don't do this first.
 * Starts the cycle counter, which is guaranteed to run until _destroy() is
 * called.
 */
static UNUSED void sel4bench_init();

/**
 * Tear down the sel4bench library. Nothing else is guaranteed to work, and may
 * produce strange failures, after you do this.
 */
static UNUSED void sel4bench_destroy();

/**
 * Query the cycle counter. If said counter needs starting, _init() will have
 * taken care of it.
 *
 * @return The current cycle count. This might be since _init(), if the cycle
 *         counter needs explicit starting, or since bootup, if it freewheels.
 */
static UNUSED ccnt_t sel4bench_get_cycle_count();

/**
 * Query how many performance counters are supported on this CPU, excluding the
 * cycle counter.
 *
 * @return Processor's available counters.
 */
static UNUSED seL4_Word sel4bench_get_num_counters();

/**
 * Query the description of a counter
 * @param counter The counter to query
 * @return An ASCII string prepresentation of the counters description, or NULL
 *         if the counter does not exist.
 */
const char* sel4bench_get_counter_description(counter_t counter);

/**
 * Query the value of a counter.
 *
 * @param counter The counter to query.
 * @return The value of the counter.
 */
static UNUSED ccnt_t sel4bench_get_counter(counter_t counter);

/**
 * Query the value of a set of counters.
 *
 * @param counters A bitfield indicating which counter(s) to query.
 * @param values   An array of ccnt_t, of length equal to the
 *                 highest counter index to be read (to a maximum of
 *                 sel4bench_get_num_counters()). Each counter to be read
 *                 will be written to its corresponding index in this array.
 * @return The current cycle count, as sel4bench_get_cycle_count()
 */
static UNUSED ccnt_t sel4bench_get_counters(counter_bitfield_t counters,
                                            ccnt_t* values);

/**
 * Assign a counter to track a specific event. Events are processor-specific,
 * though some common ones might be exposed through preprocessor constants.
 *
 * @param counter The counter to configure.
 * @param event The event to track.
 */
static UNUSED void sel4bench_set_count_event(counter_t counter, event_id_t id);

/**
 * Start counting events on a set of performance counters. The argument is a
 * bitfield detailing that set. (Note that this means the library supports a
 * number of counters less than or equal to the machine word size in bits.)
 *
 * @param counters A bitfield indicating which counter(s) to start.
 */
static UNUSED void sel4bench_start_counters(counter_bitfield_t counters);

/**
 * Stop counting events on a set of performance counters. The argument is a
 * bitfield detailing that set. (Note that this means the library supports a
 * number of counters less than or equal to the machine word size in bits.)
 *
 * Some processors (notably, the ARM1136) may not support this operation.
 *
 * @param counters A bitfield indicating which counter(s) to stop.
 */
static UNUSED void sel4bench_stop_counters(counter_bitfield_t counters);

/**
 * Reset all performance counters to zero.
 *
 */
static UNUSED void sel4bench_reset_counters(void);

/*
 * @return the number benchmark loops required to read a number of events
 */
static inline int
sel4bench_get_num_counter_chunks(seL4_Word n_counters, seL4_Word n_events)
{
    return DIV_ROUND_UP(n_events, n_counters);
}

/*
 * Enable a chunk of the event counters passed in.
 *
 * @param chunk      events is divided into sel4bench_get_num_counter_chunks, determined by
 *                   ceiling(n_events / n_counters).
 * @param n_counters value returned by sel4bench_get_num_counters(), passed in to avoid
 *                         expensive operations multiple times.
 * @return a mask    used to manipulate the counters enabled.
 */
static inline counter_bitfield_t
sel4bench_enable_counters(seL4_Word n_events, event_id_t events[n_events], seL4_Word chunk,
                          seL4_Word n_counters)
{
    assert(chunk < sel4bench_get_num_counter_chunks(n_counters, n_events));
    /* This value is passed in as it can be expensive to call, but
     * should not be different to the returned function result */
    assert(n_counters == sel4bench_get_num_counters());
    counter_bitfield_t mask = 0;

    for (seL4_Word i = 0; i < n_counters; i++) {
        seL4_Word counter = chunk * n_counters + i;
        if (counter >= n_events) {
            break;
        }
        sel4bench_set_count_event(i, events[counter]);
        mask |= BIT(i);
    }

    sel4bench_reset_counters();
    sel4bench_start_counters(mask);
    return mask;
}

/*
 * Read and stop the counters set in mask.
 *
 * @param chunk      as passed to sel4bench_enable_counters.
 * @param n_counters value returned by sel4bench_get_num_counters(), passed in to avoid
 *                         expensive operations multiple times.
 * @param mask       returned by sel4bench_enable_counters.
 * @param results    array of counter results. Must match the size of n_events as passed to
 *                   sel4bench_enable_counters.
 */
static inline void
sel4bench_read_and_stop_counters(counter_bitfield_t mask, seL4_Word chunk, seL4_Word n_counters,
                                 ccnt_t results[])
{
   sel4bench_get_counters(mask, &results[chunk * n_counters]);
   sel4bench_stop_counters(mask);
}

/* shortcut for calling sel4bench_enable_counters on the GENERIC_EVENTS supplied by all platforms in
 * libsel4bench */
static inline counter_bitfield_t
sel4bench_enable_generic_counters(seL4_Word chunk, seL4_Word n_counters) {
    return sel4bench_enable_counters(SEL4BENCH_NUM_GENERIC_EVENTS, GENERIC_EVENTS, chunk, n_counters);
}

/* shortcut for calling sel4bench_enable_counters on the GENERIC_EVENTS supplied by all platforms in
 * libsel4bench */
static inline int
sel4bench_get_num_generic_counter_chunks(seL4_Word n_counters) {
    return sel4bench_get_num_counter_chunks(n_counters, SEL4BENCH_NUM_GENERIC_EVENTS);
}

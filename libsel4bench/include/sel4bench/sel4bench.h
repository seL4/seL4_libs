/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
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
 * injection calls to make them happen.  As a result, expect that any library
 * call could potentially result in a syscall.
 *
 * It also goes out of its way to ensure that there's always a cycle counter
 * available for use.  `sel4bench_init()` will start this running, and
 * `sel4bench_destroy()` will tear it down, if necessary.
 *
 * Notes:
 * - Overflow is completely ignored, even on processors that only support
 *   32-bit counters (and thus where there is space to overflow into).  If you
 *   are doing something that might overflow a counter, it's up to you to deal
 *   with that possibility.
 * - Everything is zero-indexed.
 */

/*
 * CPP constants for events that are common to all architecture variants.
 *
 * Additional events are architecture- (and potentially processor-) specific.
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

static UNUSED char *GENERIC_EVENT_NAMES[] = {
    "L1 i-cache misses",
    "L1 d-cache misses",
    "L1 i-tlb misses",
    "L1 d-tlb misses",
    "Instructions",
    "Branch mispredictions",
    "Memory accesses",
};

static_assert(ARRAY_SIZE(GENERIC_EVENTS) == ARRAY_SIZE(GENERIC_EVENT_NAMES),
              "event names same length as counters");

/* Number of generic counters */
#define SEL4BENCH_NUM_GENERIC_EVENTS ARRAY_SIZE(GENERIC_EVENTS)

/**
 * Initialise the sel4bench library.  Nothing else is guaranteed to work, and
 * may produce strange failures, if you don't do this first.
 *
 * Starts the cycle counter, which is guaranteed to run until
 * `sel4bench_destroy()` is called.
 */
static UNUSED void sel4bench_init();

/**
 * Tear down the sel4bench library.  Nothing else is guaranteed to work, and may
 * produce strange failures, after you do this.
 */
static UNUSED void sel4bench_destroy();

/**
 * Query the cycle counter.  If said counter needs starting, `sel4bench_init()`
 * will have taken care of it.
 *
 * The returned cycle count might be since `sel4bench_init()`, if the cycle
 * counter needs explicit starting, or since bootup, if it freewheels.
 *
 * @return current cycle count
 */
static UNUSED ccnt_t sel4bench_get_cycle_count();

/**
 * Query how many performance counters are supported on this CPU, excluding the
 * cycle counter.
 *
 * Note that the return value is of type `seL4_Word`; consequently, this library
 * supports a number of counters less than or equal to the machine word size in
 * bits.

 * @return quantity of counters on this CPU
 */
static UNUSED seL4_Word sel4bench_get_num_counters();

/**
 * Query the description of a counter.
 *
 * @param counter counter to query
 *
 * @return ASCII string representation of counter's description; `NULL` if
 * counter does not exist
 */
const char *sel4bench_get_counter_description(counter_t counter);

/**
 * Query the value of a counter.
 *
 * @param counter counter to query
 *
 * @return counter value
 */
static UNUSED ccnt_t sel4bench_get_counter(counter_t counter);

/**
 * Query the value of a set of counters.
 *
 * `values` must point to an array of a length at least equal to the highest
 * counter index to be read (to a maximum of `sel4bench_get_num_counters()`).
 * Each counter to be read will be written to its corresponding index in this
 * array.
 *
 * @param counters bitfield indicating which counter(s) in `values` to query
 * @param values   array of counters
 *
 * @return current cycle count as in `sel4bench_get_cycle_count()`
 */
static UNUSED ccnt_t sel4bench_get_counters(counter_bitfield_t counters,
                                            ccnt_t *values);

/**
 * Assign a counter to track a specific event.  Events are processor-specific,
 * though some common ones might be exposed through preprocessor constants.
 *
 * @param counter counter to configure
 * @param event   event to track
 */
static UNUSED void sel4bench_set_count_event(counter_t counter, event_id_t id);

/**
 * Start counting events on a set of performance counters.
 *
 * @param counters bitfield indicating which counter(s) to start
 */
static UNUSED void sel4bench_start_counters(counter_bitfield_t counters);

/**
 * Stop counting events on a set of performance counters.
 *
 * Note: Some processors may not support this operation.
 *
 * @param counters bitfield indicating which counter(s) to stop
 */
static UNUSED void sel4bench_stop_counters(counter_bitfield_t counters);

/**
 * Reset all performance counters to zero.  Note that the cycle counter is not a
 * performance counter, and is not reset.
 *
 */
static UNUSED void sel4bench_reset_counters(void);

/**
 * Query the number of benchmark loops required to read a given number of
 * events.
 *
 * @param n_counters number of counters available
 * @param n_events   number of events of interest
 *
 * @return number of benchmark loops required
 */
static inline int sel4bench_get_num_counter_chunks(seL4_Word n_counters,
                                                   seL4_Word n_events)
{
    return DIV_ROUND_UP(n_events, n_counters);
}

/**
 * Enable a chunk of the event counters passed in.
 *
 * A "chunk" is a quantity of events not larger than the number of performance
 * counters available.  Because we can be interested in more events than there
 * are counters, the events are broken into numbered chunks (zero-indexed).  The
 * quantity of chunks is ceil(n_events / n_counters).
 *
 * Imagine we had 10 events to track but n_counters was only 8 (i.e., an 8-bit
 * machine).
 *
 *     +--chunk 1-+--chunk 0-+
 *     | xxxxxxxx | xxxxxxxx |
 *     +---------------------+
 *
 * sel4bench_enable_counters(10, events, 0, 8) would return 255:
 *
 *     +--chunk 1-+--chunk 0-+
 *     | 00000000 | 11111111 |
 *     +---------------------+
 *
 * sel4bench_enable_counters(10, events, 1, 8) would return 3:
 *
 *     +--chunk 1-+--chunk 0-+
 *     | 00000011 | 00000000 |
 *     +---------------------+
 *
 * `n_counters` is a parameter because calling `sel4bench_get_num_counters()`
 * can be expensive, but it should be the same as the function's return value.
 *
 * @param n_events   number of events of interest
 * @param event      events to track
 * @param chunk      chunk number to enable
 * @param n_counters number of counters available
 *
 * @return mask usable to manipulate the counters enabled
 */
static inline
counter_bitfield_t sel4bench_enable_counters(seL4_Word n_events,
                                             event_id_t *events,
                                             seL4_Word chunk,
                                             seL4_Word n_counters)
{
    assert(chunk < sel4bench_get_num_counter_chunks(n_counters, n_events));
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

/**
 * Read and stop the counters set in `mask`.
 *
 * `n_counters` is a parameter because calling `sel4bench_get_num_counters()`
 * can be expensive, but it should be the same as the function's return value.
 *
 * `results` must point to an array the size of n_events, as passed to
 * `sel4bench_enable_counters()`.
 *
 * @param mask       as returned by `sel4bench_enable_counters()`
 * @param chunk      as passed to `sel4bench_enable_counters()`
 * @param n_counters number of counters available
 * @param results    array of counter results
 */
static inline void sel4bench_read_and_stop_counters(counter_bitfield_t mask,
                                                    seL4_Word chunk,
                                                    seL4_Word n_counters,
                                                    ccnt_t results[])
{
    sel4bench_get_counters(mask, &results[chunk * n_counters]);
    sel4bench_stop_counters(mask);
}

/**
 * Call `sel4bench_enable_counters()` on the `GENERIC_EVENTS` supplied for all
 * platforms by this library.
 *
 * See `sel4bench_enable_counters()` for parameters and return value.
 */
static inline counter_bitfield_t sel4bench_enable_generic_counters(
    seL4_Word chunk, seL4_Word n_counters)
{
    return sel4bench_enable_counters(SEL4BENCH_NUM_GENERIC_EVENTS,
                                     GENERIC_EVENTS, chunk, n_counters);
}

/**
 * Call `sel4bench_get_num_counter_chunks()` for the `GENERIC_EVENTS` supplied
 * for all platforms by this library.
 *
 * See `sel4bench_get_num_counter_chunks()` for parameters and return value.
 */
static inline int sel4bench_get_num_generic_counter_chunks(seL4_Word n_counters)
{
    return sel4bench_get_num_counter_chunks(n_counters,
                                            SEL4BENCH_NUM_GENERIC_EVENTS);
}

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __SEL4BENCH_H__
#define __SEL4BENCH_H__

#include <stddef.h>
#include <stdint.h>

#include <sel4/sel4.h>
#include <sel4bench/arch/sel4bench.h>

#define SEL4BENCH_API static __attribute__((unused))

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
 * Finally, some CPP constants for events that are common to all architecture
 * variants are also exposed. These are:
 * <ul>
 *  <li> SEL4BENCH_EVENT_CACHE_L1I_MISS      </li>
 *  <li> SEL4BENCH_EVENT_CACHE_L1D_MISS      </li>
 *  <li> SEL4BENCH_EVENT_TLB_L1I_MISS        </li>
 *  <li> SEL4BENCH_EVENT_TLB_L1D_MISS        </li>
 *  <li> SEL4BENCH_EVENT_MEMORY_ACCESS       </li>
 *  <li> SEL4BENCH_EVENT_EXECUTE_INSTRUCTION </li>
 *  <li> SEL4BENCH_EVENT_BRANCH_MISPREDICT   </li>
 * </ul>
 * Additional events are architecture (and potentially processor) specific.
 * These may be defined in architecture or processor header files.
 * 
 * See the events.h for your architecture for specific events, caveats,
 * gotchas, and other trickery.
 * 
 * Notes:
 * - The MEMORY_ACCESS event refers to all memory loads/stores issued by the
 *   processor -- that is, all memory accesses *except* instruction fetch
 *   operations -- whether they are satisfied by a cache or from main memory.
 * - Overflow is completely ignored, even on processors that only support
 *   32-bit counters (and thus where there is space to overflow into). If you
 *   are doing something that might overflow a counter, it's up to you to deal
 *   with that possibility.
 * - Everything is zero-indexed.
 */


/**
 * Initialise the sel4bench library. Nothing else is guaranteed to work, and
 * may produce strange failures, if you don't do this first.
 * Starts the cycle counter, which is guaranteed to run until _destroy() is
 * called.
 */
SEL4BENCH_API void sel4bench_init();

/**
 * Tear down the sel4bench library. Nothing else is guaranteed to work, and may
 * produce strange failures, after you do this.
 */
SEL4BENCH_API void sel4bench_destroy();

/**
 * Query the cycle counter. If said counter needs starting, _init() will have
 * taken care of it.
 * 
 * @return The current cycle count. This might be since _init(), if the cycle
 *         counter needs explicit starting, or since bootup, if it freewheels.
 */
SEL4BENCH_API sel4bench_counter_t sel4bench_get_cycle_count();

/**
 * Query how many performance counters are supported on this CPU, excluding the
 * cycle counter.
 * 
 * @return Processor's available counters.
 */
SEL4BENCH_API seL4_Word sel4bench_get_num_counters();

/**
 * Query the value of a counter.
 * 
 * @param counter The counter to query.
 * @return The value of the counter.
 */
SEL4BENCH_API sel4bench_counter_t sel4bench_get_counter(seL4_Word counter);

/**
 * Query the value of a set of counters.
 * 
 * @param counters A bitfield indicating which counter(s) to query.
 * @param values   An array of sel4bench_counter_t, of length equal to the
 *                 highest counter index to be read (to a maximum of
 *                 sel4bench_get_num_counters()). Each counter to be read
 *                 will be written to its corresponding index in this array.
 * @return The current cycle count, as sel4bench_get_cycle_count()
 */
SEL4BENCH_API sel4bench_counter_t sel4bench_get_counters(seL4_Word counters, sel4bench_counter_t* values);

/**
 * Assign a counter to track a specific event. Events are processor-specific,
 * though some common ones might be exposed through preprocessor constants.
 * 
 * @param counter The counter to configure.
 * @param event The event to track.
 */
SEL4BENCH_API void sel4bench_set_count_event(seL4_Word counter, seL4_Word event);

/**
 * Start counting events on a set of performance counters. The argument is a
 * bitfield detailing that set. (Note that this means the library supports a
 * number of counters less than or equal to the machine word size in bits.)
 * 
 * @param counters A bitfield indicating which counter(s) to start.
 */
SEL4BENCH_API void sel4bench_start_counters(seL4_Word counters);

/**
 * Stop counting events on a set of performance counters. The argument is a
 * bitfield detailing that set. (Note that this means the library supports a
 * number of counters less than or equal to the machine word size in bits.)
 * 
 * Some processors (notably, the ARM1136) may not support this operation.
 * 
 * @param counters A bitfield indicating which counter(s) to stop.
 */
SEL4BENCH_API void sel4bench_stop_counters(seL4_Word counters);

/**
 * Reset a set of performance counters to zero. The argument is a
 * bitfield detailing that set. (Note that this means the library supports a
 * number of counters less than or equal to the machine word size in bits.)
 * 
 * @param counters A bitfield indicating which counter(s) to reset.
 */
SEL4BENCH_API void sel4bench_reset_counters(seL4_Word counters);

#endif /* __SEL4BENCH_H__ */


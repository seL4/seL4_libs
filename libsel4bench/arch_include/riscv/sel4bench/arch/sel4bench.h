/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <autoconf.h>
#include <stdlib.h>
#include <sel4bench/types.h>
#include <sel4/sel4.h>
#include <utils/util.h>

#define FASTFN inline __attribute__((always_inline))

#if __riscv_xlen == 32
#define SEL4BENCH_READ_CCNT(var) \
    do { \
        uint32_t nH1, nL, nH2; \
        asm volatile("rdcycleh %0\n" \
                    "rdcycle %1\n" \
                    "rdcycleh %2\n" \
                    : "=r"(nH1), "=r"(nL), "=r"(nH2)); \
        if (nH1 < nH2) { \
            asm volatile("rdcycle %0" : "=r"(nL)); \
            nH1 = nH2; \
        } \
        var = ((uint64_t)nH1 << 32) | nL; \
    } while(0)
#else
#define SEL4BENCH_READ_CCNT(var) \
    asm volatile("rdcycle %0" :"=r"(var));
#endif

#define SEL4BENCH_RESET_CCNT do {\
    ; \
} while(0)

#if __riscv_xlen == 32
#define SEL4BENCH_READ_PCNT(idx, var) \
    do { \
        uint32_t nH1, nL, nH2; \
        asm volatile("csrr %0, hpmcounterh" #idx \
                    "csrr %1, hpmcounter" #idx \
                    "csrr %2, hpmcounterh" #idx \
                    : "=r"(nH1), "=r"(nL), "=r"(nH2)); \
        if (nH1 < nH2) { \
            asm volatile("csrr %0, hpmcounter" #idx : "=r"(nL)); \
            nH1 = nH2; \
        } \
        var = ((uint64_t)nH1 << 32) | nL; \
    } while(0)
#else
#define SEL4BENCH_READ_PCNT(idx, var) \
    asm volatile("csrr %0, hpmcounter" #idx : "=r"(var));
#endif

/* Check out SiFive FU540 Manual Chapter 4.10 for details
 * These settings are platform specific, however, they
 * might become part of the RISCV spec in the future.
 */
#define SEL4BENCH_EVENT_EXECUTE_INSTRUCTION 0x3FFFF00
#define SEL4BENCH_EVENT_CACHE_L1I_MISS      0x102
#define SEL4BENCH_EVENT_CACHE_L1D_MISS      0x202
#define SEL4BENCH_EVENT_TLB_L1I_MISS        0x802
#define SEL4BENCH_EVENT_TLB_L1D_MISS        0x1002
#define SEL4BENCH_EVENT_BRANCH_MISPREDICT   0x6001
#define SEL4BENCH_EVENT_MEMORY_ACCESS       0x202

#define CCNT_FORMAT "%"PRIu64
typedef uint64_t ccnt_t;

static FASTFN void sel4bench_init()
{
    /* Nothing to do */
}

static FASTFN void sel4bench_destroy()
{
    /* Nothing to do */
}

static FASTFN seL4_Word sel4bench_get_num_counters()
{
#ifdef CONFIG_PLAT_HIFIVE
    return 2;
#else
    return 0;
#endif
}

static FASTFN ccnt_t sel4bench_get_cycle_count()
{
    ccnt_t val;

    SEL4BENCH_READ_CCNT(val);

    return val;
}

/* Being declared FASTFN allows this function (once inlined) to cache miss; I
 * think it's worthwhile in the general case, for performance reasons.
 * moreover, it's small enough that it'll be suitably aligned most of the time
 */
static FASTFN ccnt_t sel4bench_get_counter(counter_t counter)
{
    ccnt_t val;

    /* Sifive U540 only supports two event counters */
    switch (counter) {
    case 0:
        SEL4BENCH_READ_PCNT(3, val);
        break;
    case 1:
        SEL4BENCH_READ_PCNT(4, val);
        break;
    default:
        val = 0;
        break;
    }

    return val;
}

static inline ccnt_t sel4bench_get_counters(counter_bitfield_t mask, ccnt_t *values)
{
    ccnt_t ccnt;
    unsigned int counter = 0;

    for (; mask != 0 ; mask >>= 1, counter++) {
        if (mask & 1) {
            values[counter] = sel4bench_get_counter(counter);
        }
    }

    SEL4BENCH_READ_CCNT(ccnt);

    return ccnt;
}

static FASTFN void sel4bench_set_count_event(counter_t counter, event_id_t event)
{
    /* Sifive U540 only supports two event counters */
    switch (counter) {
    case 0:
        /* Stop the counter */
        asm volatile("csrw mhpmevent3, 0");

        /* Reset and start the counter*/
#if __riscv_xlen == 32
        asm volatile("csrw mhpmcounterh3, 0");
#endif
        asm volatile("csrw mhpmcounter3, 0\n"
                     "csrw mhpmevent3, %0\n"
                     :: "r"(event));
        break;
    case 1:
        asm volatile("csrw mhpmevent4, 0");
#if __riscv_xlen == 32
        asm volatile("csrw mhpmcounterh4, 0");
#endif
        asm volatile("csrw mhpmcounter4, 0\n"
                     "csrw mhpmevent4, %0\n"
                     :: "r"(event));
        break;
    default:
        break;
    }

    return;
}

/* Writing the to event CSR would automatically start the counter */
static FASTFN void sel4bench_start_counters(counter_bitfield_t mask)
{
    /* Nothing to do */
}

/* Note that the counter is stopped by clearing the event CSR.
 * Set event CSR before starting the counter again
 */
static FASTFN void sel4bench_stop_counters(counter_bitfield_t mask)
{
    /* Sifive U540 only supports two event counters */
    if (mask & (1 << 3)) {
        asm volatile("csrw mhpmevent3, 0");
    }

    if (mask & (1 << 4)) {
        asm volatile("csrw mhpmevent4, 0");
    }
    return;
}

static FASTFN void sel4bench_reset_counters(void)
{
    /* Nothing to do */
}

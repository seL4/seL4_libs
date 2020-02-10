/*
 * Copyright 2020, Data61
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

#include <autoconf.h>
#include <sel4bench/types.h>
#include <sel4/sel4.h>
#include <utils/util.h>

#define FASTFN inline __attribute__((always_inline))

#define SEL4BENCH_READ_CCNT(var) asm volatile("rdcycle %0" :"=r"(var));  

#define SEL4BENCH_RESET_CCNT do {\
    ; \
} while(0)

#define SEL4BENCH_EVENT_EXECUTE_INSTRUCTION 0x00
#define SEL4BENCH_EVENT_CACHE_L1I_MISS 0xf0
#define SEL4BENCH_EVENT_CACHE_L1D_MISS 0xf1
#define SEL4BENCH_EVENT_TLB_L1I_MISS   0xf2
#define SEL4BENCH_EVENT_TLB_L1D_MISS   0xf3
#define SEL4BENCH_EVENT_BRANCH_MISPREDICT   0xf4
#define SEL4BENCH_EVENT_MEMORY_ACCESS       0xf5

#define CCNT_FORMAT "%"PRIu64
typedef uint64_t ccnt_t;

static FASTFN void sel4bench_init()
{
}

static FASTFN void sel4bench_destroy()
{
}

static FASTFN seL4_Word sel4bench_get_num_counters()
{
    return 0;
}

static FASTFN ccnt_t sel4bench_get_cycle_count()
{
    ccnt_t val;
    return val;
}

/* Being declared FASTFN allows this function (once inlined) to cache miss; I
 * think it's worthwhile in the general case, for performance reasons.
 * moreover, it's small enough that it'll be suitably aligned most of the time
 */
static FASTFN ccnt_t sel4bench_get_counter(counter_t counter)
{
    uint32_t val = 0;
    return val;
}

/* This reader function is too complex to be inlined, so we force it to be
 * cacheline-aligned in order to avoid icache misses with the counters off.
 * (relevant note: GCC compiles this function to be exactly one ARMV7 cache
 * line in size) however, the pointer dereference is overwhelmingly likely to
 * produce a dcache miss, which will occur with the counters off
 */
static inline ccnt_t sel4bench_get_counters(counter_bitfield_t mask, ccnt_t *values)
{
    return 0;
}

static FASTFN void sel4bench_set_count_event(counter_t counter, event_id_t event)
{
    return;
}

static FASTFN void sel4bench_start_counters(counter_bitfield_t mask)
{
    return;
}

static FASTFN void sel4bench_stop_counters(counter_bitfield_t mask)
{
    return;
}

static FASTFN void sel4bench_reset_counters(void)
{
    return;
}

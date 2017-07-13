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

#include <stdint.h>
#include <sel4bench/cpu/events.h>

//function attributes
//functions that need to be forced inline
#define FASTFN inline __attribute__((always_inline))

//functions that must not cache miss
#define CACHESENSFN __attribute__((noinline, aligned(32)))

//functions that will be called through pointers, but need to be fast
#define KERNELFN __attribute__((noinline, flatten))

//counters and related constants
#define SEL4BENCH_ARM1136_NUM_COUNTERS 2

#define SEL4BENCH_ARM1136_COUNTER_CCNT "1"
#define SEL4BENCH_ARM1136_COUNTER_PMN0 "2"
#define SEL4BENCH_ARM1136_COUNTER_PMN1 "3"

/*
 * PMNC: ARM1136 Performance Monitor Control Register
 *
 *  bits 31:28 = SBZ
 *  bits 27:20 = EvtCount1 = event monitored by counter 1
 *  bits 19:12 = EvtCount2 = event monitored by counter 2
 *  bit     11 = X         = export events to ETM
 *  bits 10: 8 = Flag      = read: determines if C0 (bit 8)/
 *                                 C1 (bit 9)/CCNT (bit 10)
 *                                 overflowed
 *                           write: clears overflow flag
 *  bit      7 = SBZ
 *  bits  6: 4 = IntEn     = Enable interrupt reporting for
 *                           C0 (bit 4)/C1 (bit 5)/CCNT (bit 6)
 *  bit      3 = D         = cycle counter divides by 64
 *  bit      2 = C         = write 1 to reset CCNT to zero
 *  bit      1 = P         = write 1 to reset C0 and C1 to zero
 *  bit      0 = E         = enable all three counters
 */
typedef union {
    struct {
        uint32_t E         : 1;
        uint32_t P         : 1;
        uint32_t C         : 1;
        uint32_t D         : 1;
        uint32_t IntEn     : 3;
        uint32_t SBZ1      : 1;
        uint32_t Flag      : 3;
        uint32_t X         : 1;
        uint32_t EvtCount2 : 8;
        uint32_t EvtCount1 : 8;
        uint32_t SBZ2      : 4;
    };
    uint32_t raw;
} sel4bench_arm1136_pmnc_t;

static CACHESENSFN void sel4bench_private_set_pmnc(sel4bench_arm1136_pmnc_t val)
{
    /*
     * The ARM1136 has a 3-cycle delay between changing the PMNC and the
     * counters reacting. So we insert 3 nops to cover for that. Aligning on a
     * cache line boundary guarantees that the nops won't cause anything
     * interesting to happen.
     */
    asm volatile (
        "mcr p15, 0, %0, c15, c12, 0;"
        "nop;"
        "nop;"
        "nop;"
        :
        : "r"(val.raw)
    );
}
static FASTFN sel4bench_arm1136_pmnc_t sel4bench_private_get_pmnc(void)
{
    sel4bench_arm1136_pmnc_t val;
    asm volatile (
        "mrc p15, 0, %0, c15, c12, 0"
        : "=r"(val.raw)
        :
    );
    return val;
}

/*
 * CCNT: cycle counter
 */
static FASTFN uint32_t sel4bench_private_get_ccnt()
{
    uint32_t val;
    asm volatile (
        "mrc p15, 0, %0, c15, c12," SEL4BENCH_ARM1136_COUNTER_CCNT
        : "=r"(val)
    );
    return val;
}

/*
 * PMN0: event count 0
 */
static FASTFN uint32_t sel4bench_private_get_pmn0()
{
    uint32_t val;
    asm volatile (
        "mrc p15, 0, %0, c15, c12,"SEL4BENCH_ARM1136_COUNTER_PMN0
        : "=r"(val)
    );
    return val;
}
static FASTFN void sel4bench_private_set_pmn0(uint32_t val)
{
    asm volatile (
        "mcr p15, 0, %0, c15, c12,"SEL4BENCH_ARM1136_COUNTER_PMN0
        : "=r"(val)
    );
}

/*
 * PMN1: event count 1
 */
static FASTFN uint32_t sel4bench_private_get_pmn1()
{
    uint32_t val;
    asm volatile (
        "mrc p15, 0, %0, c15, c12,"SEL4BENCH_ARM1136_COUNTER_PMN1
        : "=r"(val)
    );
    return val;
}
static FASTFN void sel4bench_private_set_pmn1(uint32_t val)
{
    asm volatile (
        "mcr p15, 0, %0, c15, c12,"SEL4BENCH_ARM1136_COUNTER_PMN1
        : "=r"(val)
    );
}

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
#include <sel4bench/armv/events.h>
#include <sel4bench/sel4_arch/sel4bench.h>
#include <utils/util.h>

//function attributes
//functions that need to be inlined for speed
#define FASTFN inline __attribute__((always_inline))
//functions that must not cache miss
#define CACHESENSFN __attribute__((noinline, aligned(64)))

//counters and related constants
#define SEL4BENCH_ARMV8A_NUM_COUNTERS 4

#define SEL4BENCH_ARMV8A_COUNTER_CCNT 31

// select whether user mode gets access to the PMCs
static FASTFN void sel4bench_private_switch_user_pmc(unsigned long state)
{
    PMU_WRITE(PMUSERENR, state);
}

/*
 * INTENS
 *
 * Enables the generation of interrupt requests on overflows from the Cycle Count Register,
 * PMCCNTR_EL0, and the event counters PMEVCNTR<n>_EL0. Reading the register shows which
 * overflow interrupt requests are enabled.
 */
static FASTFN void sel4bench_private_write_intens(uint32_t mask)
{
    PMU_WRITE(PMINTENSET, mask);
}

/*
 * INTENC
 *
 * Disables the generation of interrupt requests on overflows from the Cycle Count Register,
 * PMCCNTR_EL0, and the event counters PMEVCNTR<n>_EL0. Reading the register shows which
 * overflow interrupt requests are enabled.
 *
 */
static FASTFN void sel4bench_private_write_intenc(uint32_t mask)
{
    PMU_WRITE(PMINTENCLR, mask);
}

static inline void sel4bench_private_init(void* data)
{
    //enable user-mode performance-counter access
    sel4bench_private_switch_user_pmc(1);

    //disable overflow interrupts on all counters
    sel4bench_private_write_intenc(-1);
}
static inline void sel4bench_private_deinit(void* data)
{
    //disable user-mode performance-counter access
    sel4bench_private_switch_user_pmc(0);
}

/*
 * PMCR:
 *
 *  bits 31:24 = implementor
 *  bits 23:16 = idcode
 *  bits 15:11 = number of counters
 *  bits 10:6  = reserved, sbz
 *  bit  5 = disable CCNT when non-invasive debug is prohibited
 *  bit  4 = export events to ETM
 *  bit  3 = cycle counter divides by 64
 *  bit  2 = write 1 to reset cycle counter to zero
 *  bit  1 = write 1 to reset all counters to zero
 *  bit  0 = enable bit
 */
#define SEL4BENCH_ARMV8A_PMCR_N(x)       (((x) & 0xFFFF) >> 11u)
#define SEL4BENCH_ARMV8A_PMCR_ENABLE     BIT(0)
#define SEL4BENCH_ARMV8A_PMCR_RESET_ALL  BIT(1)
#define SEL4BENCH_ARMV8A_PMCR_RESET_CCNT BIT(2)
#define SEL4BENCH_ARMV8A_PMCR_DIV64      BIT(3) /* Should CCNT be divided by 64? */

static FASTFN void sel4bench_private_write_pmcr(uint32_t val)
{
    PMU_WRITE(PMCR, val);
}
static FASTFN uint32_t sel4bench_private_read_pmcr(void)
{
    uint32_t val;
    PMU_READ(PMCR, val);
    return val;
}

/*
 * CNTENS/CNTENC (Count Enable Set/Clear)
 *
 * Enables the Cycle Count Register, PMCCNTR_EL0, and any implemented event counters
 * PMEVCNTR<x>. Reading this register shows which counters are enabled.
 *
 */
static FASTFN void sel4bench_private_write_cntens(uint32_t mask)
{
    PMU_WRITE(PMCNTENSET, mask);
}

static FASTFN uint32_t sel4bench_private_read_cntens(void)
{
    uint32_t mask;
    PMU_READ(PMCNTENSET, mask);
    return mask;
}

/*
 * Disables the Cycle Count Register, PMCCNTR_EL0, and any implemented event counters
 * PMEVCNTR<x>. Reading this register shows which counters are enabled.
 */
static FASTFN void sel4bench_private_write_cntenc(uint32_t mask)
{
    PMU_WRITE(PMCNTENCLR, mask);
}

/*
 * Reads or writes the value of the selected event counter, PMEVCNTR<n>_EL0.
 * PMSELR_EL0.SEL determines which event counter is selected.
 */
static FASTFN uint32_t sel4bench_private_read_pmcnt(void)
{
    uint32_t val;
    PMU_READ(PMXEVCNTR, val);
    return val;
}

static FASTFN void sel4bench_private_write_pmcnt(uint32_t val)
{
    PMU_WRITE(PMXEVCNTR, val);
}

/*
 * Selects the current event counter PMEVCNTR<x> or the cycle counter, CCNT
 */
static FASTFN void sel4bench_private_write_pmnxsel(uint32_t val)
{
    PMU_WRITE(PMSELR, val);
}

/*
 * When PMSELR_EL0.SEL selects an event counter, this accesses a PMEVTYPER<n>_EL0
 * register. When PMSELR_EL0.SEL selects the cycle counter, this accesses PMCCFILTR_EL0.
 */
static FASTFN uint32_t sel4bench_private_read_evtsel(void)
{

    uint32_t val;
    PMU_READ(PMXEVTYPER, val);
    return val;
}

static FASTFN void sel4bench_private_write_evtsel(uint32_t val)
{
    PMU_WRITE(PMXEVTYPER, val);
}

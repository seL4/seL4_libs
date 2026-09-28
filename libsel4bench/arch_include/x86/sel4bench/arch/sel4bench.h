/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <sel4bench/types.h>

#define SEL4BENCH_READ_CCNT(var) do { \
    (var) = sel4bench_get_cycle_count(); \
} while(0)

/* Intel docs are somewhat unclear as to exactly how to serialize PMCs.
 * Using LFENCE for the moment, because it's much faster. If event counts
 * turn out to be unreliable, switch to CPUID by uncommenting this line.
 *
 * This currently breaks the GCC register allocator.
 */
//#define SEL4BENCH_STRICT_PMC_SERIALIZATION

#include <sel4bench/arch/private.h>

#define CCNT_FORMAT "%"PRIu64
typedef uint64_t ccnt_t;

/* The framework as it stands supports the following Intel processors:
 * - All P6-family processors (up to and including the Pentium M)
 * - All processors supporting IA-32 architectural performance
 *   monitoring (that is, processors starting from the Intel Core Solo,
 *   codenamed Yonah)
 */

/* Silence warnings about including the following functions when seL4_DebugRun
 * is not enabled when we are not calling them. If we actually call these
 * functions without seL4_DebugRun enabled, we'll get a link failure, so this
 * should be OK.
 */
void seL4_DebugRun(void (* userfn) (void *), void* userarg);

static inline uint64_t sel4bench_x86_rdmsr(uint32_t reg) {
#ifdef CONFIG_KERNEL_X86_DANGEROUS_MSR
    return seL4_X86DangerousRDMSR(reg);
#else
    uint32_t msr_data[3];
    msr_data[0] = reg;
    msr_data[1] = 0;
    msr_data[2] = 0;

    seL4_DebugRun(&sel4bench_private_rdmsr, msr_data);
    return (uint64_t)msr_data[1] + ((uint64_t)msr_data[2] << 32);
#endif
}

static inline void sel4bench_x86_wrmsr(uint32_t reg, uint64_t val) {
#ifdef CONFIG_KERNEL_X86_DANGEROUS_MSR
    seL4_X86DangerousWRMSR(reg, val);
#else
    uint32_t msr_data[3];
    msr_data[0] = reg;
    msr_data[1] = val & 0xffffffff;
    msr_data[2] = val >> 32;

    seL4_DebugRun(&sel4bench_private_wrmsr, msr_data);
#endif
}

static FASTFN void sel4bench_init()
{
    uint32_t cpuid_eax;
    uint32_t cpuid_ebx;
    uint32_t cpuid_ecx;
    uint32_t cpuid_edx;
    sel4bench_private_cpuid(IA32_CPUID_LEAF_BASIC, 0, &cpuid_eax, &cpuid_ebx, &cpuid_ecx, &cpuid_edx);

    //check we're running on an Intel chip
    assert(cpuid_ebx == IA32_CPUID_BASIC_MAGIC_EBX && cpuid_ecx == IA32_CPUID_BASIC_MAGIC_ECX && cpuid_edx == IA32_CPUID_BASIC_MAGIC_EDX);

    //check that either we support architectural performance monitoring, or we're running on a P6-class chip
    if (cpuid_eax < IA32_CPUID_LEAF_PMC) { //basic CPUID invocation tells us whether the processor supports arch PMCs
        //if not, ensure we're on a P6-family processor
        ia32_cpuid_model_info_t cpuid_model_info;
        sel4bench_private_cpuid(IA32_CPUID_LEAF_MODEL, 0, &(cpuid_model_info.raw), &cpuid_ebx, &cpuid_ecx, &cpuid_edx);
        assert(FAMILY(cpuid_model_info) == IA32_CPUID_FAMILY_P6);
        if (!(FAMILY(cpuid_model_info) == IA32_CPUID_FAMILY_P6)) {
            return;
        }
    }

    //enable user-mode RDPMC
#ifndef CONFIG_EXPORT_PMC_USER
    seL4_DebugRun(&sel4bench_private_enable_user_pmc, NULL);
#endif

    /* Enable the cycle counter
     * Documents referenced:
     * 1. Intel® 64 and IA-32 Architectures Software Developer’s Manual
     *    Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4
     *    Order Number: 325462-091US March 2026
     */
    sel4bench_private_cpuid(IA32_CPUID_LEAF_PMC, 0, &cpuid_eax, &cpuid_ebx, &cpuid_ecx, &cpuid_edx);
    uint8_t version = cpuid_eax & 0xFFUL;
    /* Need at least version 2 for fixed-function performance counter register.
     * Section "22.2.2 Architectural Performance Monitoring Version 2"
     * Chapter "PERFORMANCE MONITORING"
     * Page "Vol. 3B 22-5" */
    if (version < 2)
    {
        ZF_LOGE("CPU does not support cycle counting: version 0x%x < required 2.\n", version);
        return;
    }

    /* We care about IA32_FIXED_CTR1, also known as CPU_CLK_UNHALTED.THREAD.
     * Description from SDM: "The CPU_CLK_UNHALTED.THREAD event counts the
     * number of core cycles while the logical processor is not in a
     * halt state."
     * Table 22-1. Association of Fixed-Function Performance Counters with Architectural Performance Events
     * Chapter "PERFORMANCE MONITORING"
     * Page "Vol. 3B 22-7"
     */
    seL4_Uint8 num_fixed_ctrs = cpuid_edx & 0x1F;
    if (num_fixed_ctrs < 2 && !(cpuid_ecx & BIT(1)))
    {
        ZF_LOGE("CPU does not support IA32_FIXED_CTR1: NUM_FIXED_CTRS %u, ECX bitmap 0x%x.\n",
                num_fixed_ctrs, cpuid_ecx);
        return;
    }

    /* Configure the counter to count both OS and user code. See layout at:
     * Figure 22-2. Layout of IA32_FIXED_CTR_CTRL MSR
     * Chapter "PERFORMANCE MONITORING"
     * Page "22-6 Vol. 3B"
     */
    seL4_Uint64 ctrl = sel4bench_x86_rdmsr(IA32_FIXED_CTR_CTRL_MSR);
    ctrl &= ~(0xFULL << 4);
    ctrl |= IA32_FIXED_CTR1_COUNT_OS | IA32_FIXED_CTR1_COUNT_USER;
    sel4bench_x86_wrmsr(IA32_FIXED_CTR_CTRL_MSR, ctrl);

    /*
     * Enable globally via IA32_PERF_GLOBAL_CTRL, see layout at:
     * Figure 22-3. Layout of IA32_PERF_GLOBAL_CTRL MSR
     * Chapter "PERFORMANCE MONITORING"
     * Page "Vol. 3B 22-7"
     */
    sel4bench_x86_wrmsr(IA32_PERF_GLOBAL_CTRL_MSR, sel4bench_x86_rdmsr(IA32_PERF_GLOBAL_CTRL_MSR) | IA32_FIXED_CTR1_EN);
}

static FASTFN ccnt_t sel4bench_get_cycle_count()
{
    uint32_t pmc_fixed = BIT(30);
    uint32_t ia32_fixed_ctr1 = 1;

    sel4bench_private_serialize_pmc(); /* Serialise all preceding instructions */
    uint64_t result = sel4bench_private_rdpmc(pmc_fixed | ia32_fixed_ctr1);
    sel4bench_private_serialize_pmc(); /* Serialise all following instructions */

    return result;
}

static FASTFN seL4_Word sel4bench_get_num_counters()
{
    uint32_t dummy;

    //make sure the processor supports the PMC CPUID leaf
    uint32_t max_basic_leaf = 0;
    sel4bench_private_cpuid(IA32_CPUID_LEAF_BASIC, 0, &max_basic_leaf, &dummy, &dummy, &dummy);
    if (max_basic_leaf >= IA32_CPUID_LEAF_PMC) { //Core Solo or later supports PMC discovery via CPUID...
        //query the processor's PMC data
        ia32_cpuid_leaf_pmc_eax_t pmc_eax;

        sel4bench_private_cpuid(IA32_CPUID_LEAF_PMC, 0, &pmc_eax.raw, &dummy, &dummy, &dummy);
        return pmc_eax.gp_pmc_count_per_core;
    } else { //P6 (including Pentium M) doesn't...
        ia32_cpuid_model_info_t model_info;

        sel4bench_private_cpuid(IA32_CPUID_LEAF_MODEL, 0, &model_info.raw, &dummy, &dummy, &dummy);
        assert(FAMILY(model_info) == IA32_CPUID_FAMILY_P6); //we only support P6 processors (P3, PM, ...)

        return 2; //2 PMCs on P6
    }
}

static FASTFN ccnt_t sel4bench_get_counter(counter_t counter)
{
    sel4bench_private_serialize_pmc();    /* Serialise all preceding instructions */
    uint64_t counter_val = sel4bench_private_rdpmc(counter);
    sel4bench_private_serialize_pmc();    /* Serialise all following instructions */

    return counter_val;
}

static CACHESENSFN ccnt_t sel4bench_get_counters(counter_bitfield_t mask, ccnt_t* values)
{
    unsigned char counter = 0;

    sel4bench_private_serialize_pmc();    /* Serialise all preceding instructions */
    for (; mask != 0; mask >>= 1, counter++)
        if (mask & 1) {
            values[counter] = sel4bench_private_rdpmc(counter);
        }

    uint64_t time = sel4bench_get_cycle_count();
    sel4bench_private_serialize_pmc();    /* Serialise all following instructions */

    return time;
}

static FASTFN void sel4bench_set_count_event(counter_t counter, event_id_t event)
{
    //one implementation, because P6 and architectural PMCs work identically

    assert(counter < sel4bench_get_num_counters());

    ia32_pmc_perfevtsel_t evtsel_msr;
    evtsel_msr.raw = sel4bench_x86_rdmsr(IA32_MSR_PMC_PERFEVTSEL_BASE + counter);

    //preserve the reserved flag, like the docs tell us
    uint32_t res_flag = evtsel_msr.res;

    //rewrite the MSR to what we want
    evtsel_msr.raw   = sel4bench_private_lookup_event(event);
    evtsel_msr.USR   = 1;
    evtsel_msr.OS    = 1;
    evtsel_msr.res   = res_flag;
    sel4bench_x86_wrmsr(IA32_MSR_PMC_PERFEVTSEL_BASE + counter, evtsel_msr.raw);
}

static FASTFN void sel4bench_start_counters(counter_bitfield_t mask)
{
    /* On P6, only the first counter has an enable flag, which controls both counters
     * simultaneously.
     * Arch PMCs are all done independently.
     */
    uint32_t dummy;

    seL4_Word num_counters = sel4bench_get_num_counters();
    if (mask == ~(0UL)) {
        mask = ((BIT(num_counters)) - 1);
    } else {
        assert((~((BIT(num_counters)) - 1) & mask) == 0);
    }

    uint32_t max_basic_leaf = 0;
    sel4bench_private_cpuid(IA32_CPUID_LEAF_BASIC, 0, &max_basic_leaf, &dummy, &dummy, &dummy);

    if (!(max_basic_leaf >= IA32_CPUID_LEAF_PMC)) {
        //we're P6, because otherwise the init() assertion would have tripped
        assert(mask == 0x3);
        if (mask == 0x3) {
            mask = 1;
        } else {
            return;
        }
    }

    counter_t counter;
    //NOT your average for loop!
    for (counter = 0; mask; counter++) {
        if (!(mask & (BIT(counter)))) {
            continue;
        }

        mask &= ~(BIT(counter));

        //read appropriate MSR
        ia32_pmc_perfevtsel_t temp;
        temp.raw = sel4bench_x86_rdmsr(IA32_MSR_PMC_PERFEVTSEL_BASE + counter);

        //twiddle enable bit
        temp.EN = 1;

        //write back appropriate MSR
        sel4bench_x86_wrmsr(IA32_MSR_PMC_PERFEVTSEL_BASE + counter, temp.raw);

        //zero the counter
        sel4bench_x86_wrmsr(IA32_MSR_PMC_PERFEVTCNT_BASE + counter, 0);
    }

}

static FASTFN void sel4bench_stop_counters(counter_bitfield_t mask)
{
    /* On P6, only the first counter has an enable flag, which controls both counters
     * simultaneously.
     * Arch PMCs are all done independently.
     */
    uint32_t dummy;

    seL4_Word num_counters = sel4bench_get_num_counters();
    if (mask == ~(0UL)) {
        mask = ((BIT(num_counters)) - 1);
    } else {
        assert((~((BIT(num_counters)) - 1) & mask) == 0);
    }

    uint32_t max_basic_leaf = 0;
    sel4bench_private_cpuid(IA32_CPUID_LEAF_BASIC, 0, &max_basic_leaf, &dummy, &dummy, &dummy);

    if (!(max_basic_leaf >= IA32_CPUID_LEAF_PMC)) {
        //we're P6, because otherwise the init() assertion would have tripped
        assert(mask == 0x3);
        mask = 1;
    }

    counter_t counter;
    //NOT your average for loop!
    for (counter = 0; mask; counter++) {
        if (!(mask & (BIT(counter)))) {
            continue;
        }

        mask &= ~(BIT(counter));

        //read appropriate MSR
        ia32_pmc_perfevtsel_t temp;
        temp.raw = sel4bench_x86_rdmsr(IA32_MSR_PMC_PERFEVTSEL_BASE + counter);

        //twiddle enable bit
        temp.EN = 0;

        //write back appropriate MSR
        sel4bench_x86_wrmsr(IA32_MSR_PMC_PERFEVTSEL_BASE + counter, temp.raw);
    }
}

static FASTFN void sel4bench_destroy()
{
    //stop all performance counters
    sel4bench_stop_counters(-1);

    //disable user-mode RDPMC
#ifndef CONFIG_EXPORT_PMC_USER
    seL4_DebugRun(&sel4bench_private_disable_user_pmc, NULL);
#endif
}

static FASTFN void sel4bench_reset_counters(void)
{
    for (int i = 0; i < sel4bench_get_num_counters(); i++) {
        sel4bench_x86_wrmsr(IA32_MSR_PMC_PERFEVTCNT_BASE + i, 0);
    }
}

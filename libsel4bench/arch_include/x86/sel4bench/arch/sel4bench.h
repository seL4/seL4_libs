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

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <sel4bench/types.h>

#define SEL4BENCH_READ_CCNT(var) do { \
    uint32_t low, high; \
    asm volatile( \
        "movl $0, %%eax \n" \
        "movl $0, %%ecx \n" \
        "cpuid \n" \
        "rdtsc \n" \
        "movl %%edx, %0 \n" \
        "movl %%eax, %1 \n" \
        "movl $0, %%eax \n" \
        "movl $0, %%ecx \n" \
        "cpuid \n" \
        : \
         "=r"(high), \
         "=r"(low) \
        : \
        : "eax", "ebx", "ecx", "edx" \
    ); \
    (var) = (((uint64_t)high) << 32ull) | ((uint64_t)low); \
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
}

static FASTFN ccnt_t sel4bench_get_cycle_count()
{
    sel4bench_private_serialize_pmc(); /* Serialise all preceding instructions */
    uint64_t time = sel4bench_private_rdtsc();
    sel4bench_private_serialize_pmc(); /* Serialise all following instructions */

    return time;
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

    uint64_t time = sel4bench_private_rdtsc();
    sel4bench_private_serialize_pmc();    /* Serialise all following instructions */

    return time;
}

static FASTFN void sel4bench_set_count_event(counter_t counter, event_id_t event)
{
    //one implementation, because P6 and architectural PMCs work identically

    assert(counter < sel4bench_get_num_counters());

    //{RD,WR}MSR support data structure
    uint32_t msr_data[3];
    msr_data[0] = IA32_MSR_PMC_PERFEVTSEL_BASE + counter;
    msr_data[1] = 0;
    msr_data[2] = 0;

    //read current event-select MSR
    seL4_DebugRun(&sel4bench_private_rdmsr, msr_data);

    //preserve the reserved flag, like the docs tell us
    uint32_t res_flag = ((ia32_pmc_perfevtsel_t)msr_data[1]).res;

    //rewrite the MSR to what we want
    ia32_pmc_perfevtsel_t evtsel_msr;
    evtsel_msr.raw   = sel4bench_private_lookup_event(event);
    evtsel_msr.USR   = 1;
    evtsel_msr.OS    = 1;
    evtsel_msr.res   = res_flag;
    msr_data[1] = evtsel_msr.raw;

    assert(evtsel_msr.event != 0);

    //write back to the event-select MSR
    seL4_DebugRun(&sel4bench_private_wrmsr, msr_data);
}

static FASTFN void sel4bench_set_count_intx_bits(counter_t counter, bool in_tx, bool in_txcp)
{
    /* The Haswell uArch enhances perfevtsel_t with bit 32 to only count cycles in a RTM
       transactional region and bit 33 to exclude cycles in a RTM transactional region
       that abort in the cycle count. */
    unsigned int in_tx_bit, in_txcp_bit;

    assert(counter < sel4bench_get_num_counters());
    assert( !(counter != 2 && in_txcp) );

    //{RD,WR}MSR support data structure
    uint32_t msr_data[3];
    msr_data[0] = IA32_MSR_PMC_PERFEVTSEL_BASE + counter;
    msr_data[1] = 0;
    msr_data[2] = 0;

    //read current event-select MSR
    seL4_DebugRun(&sel4bench_private_rdmsr, msr_data);

    in_tx_bit = (in_tx ? IN_TX_BIT : 0);
    in_txcp_bit = (in_txcp ? IN_TXCP_BIT : 0);
    msr_data[2] &= (~(IN_TX_BIT)) & (~(IN_TXCP_BIT));
    msr_data[2] |= in_tx_bit | in_txcp_bit;

    //write back to the event-select MSR
    seL4_DebugRun(&sel4bench_private_wrmsr, msr_data);
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

    //{RD,WR}MSR support data structure
    uint32_t msr_data[3];

    counter_t counter;
    //NOT your average for loop!
    for (counter = 0; mask; counter++) {
        if (!(mask & (BIT(counter)))) {
            continue;
        }

        mask &= ~(BIT(counter));

        //read appropriate MSR
        msr_data[0] = IA32_MSR_PMC_PERFEVTSEL_BASE + counter;
        msr_data[1] = 0;
        msr_data[2] = 0;
        seL4_DebugRun(&sel4bench_private_rdmsr, msr_data);

        //twiddle enable bit
        ia32_pmc_perfevtsel_t temp = { .raw = msr_data[1] };
        temp.EN = 1;
        msr_data[1] = temp.raw;

        //write back appropriate MSR
        seL4_DebugRun(&sel4bench_private_wrmsr, msr_data);

        //zero the counter
        msr_data[0] = IA32_MSR_PMC_PERFEVTCNT_BASE + counter;
        msr_data[1] = 0;
        msr_data[2] = 0;
        seL4_DebugRun(&sel4bench_private_wrmsr, msr_data);
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

    //{RD,WR}MSR support data structure
    uint32_t msr_data[3];

    counter_t counter;
    //NOT your average for loop!
    for (counter = 0; mask; counter++) {
        if (!(mask & (BIT(counter)))) {
            continue;
        }

        mask &= ~(BIT(counter));

        //read appropriate MSR
        msr_data[0] = IA32_MSR_PMC_PERFEVTSEL_BASE + counter;
        msr_data[1] = 0;
        msr_data[2] = 0;
        seL4_DebugRun(&sel4bench_private_rdmsr, msr_data);

        //twiddle enable bit
        ia32_pmc_perfevtsel_t temp = { .raw = msr_data[1] };
        temp.EN = 0;
        msr_data[1] = temp.raw;

        //write back appropriate MSR
        seL4_DebugRun(&sel4bench_private_wrmsr, msr_data);
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
    uint32_t msr_data[3];
    msr_data[0] = IA32_MSR_PMC_PERFEVTCNT_BASE;
    msr_data[1] = 0;
    msr_data[2] = 0;

    for (int i = 0; i < sel4bench_get_num_counters(); i++) {
        seL4_DebugRun(&sel4bench_private_wrmsr, msr_data);
        msr_data[0]++;
    }
}

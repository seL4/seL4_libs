/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __ARCH_ARMV7A_SEL4BENCH_H__
#define __ARCH_ARMV7A_SEL4BENCH_H__

#include "sel4bench_private_armv7a.h"

//expose common event definitions
#define SEL4BENCH_EVENT_CACHE_L1I_MISS      SEL4BENCH_ARMV7A_EVENT_CACHE_L1I_MISS
#define SEL4BENCH_EVENT_CACHE_L1D_MISS      SEL4BENCH_ARMV7A_EVENT_CACHE_L1D_MISS
#define SEL4BENCH_EVENT_TLB_L1I_MISS        SEL4BENCH_ARMV7A_EVENT_TLB_L1I_MISS
#define SEL4BENCH_EVENT_TLB_L1D_MISS        SEL4BENCH_ARMV7A_EVENT_TLB_L1D_MISS
#define SEL4BENCH_EVENT_MEMORY_ACCESS       SEL4BENCH_ARMV7A_EVENT_MEMORY_ACCESS
#define SEL4BENCH_EVENT_EXECUTE_INSTRUCTION SEL4BENCH_ARMV7A_EVENT_EXECUTE_INSTRUCTION
#define SEL4BENCH_EVENT_BRANCH_MISPREDICT   SEL4BENCH_ARMV7A_EVENT_BRANCH_MISPREDICT


//utility macros
#ifndef BIT
#define BIT(x) (1ul << (x))
#endif
#define MODIFY_PMCR(op, val) sel4bench_private_write_pmcr(sel4bench_private_read_pmcr() op (val))

/* Silence warnings about including the following functions when seL4_DebugRun
 * is not enabled when we are not calling them. If we actually call these
 * functions without seL4_DebugRun enabled, we'll get a link failure, so this
 * should be OK.
 */
void seL4_DebugRun(void (* userfn) (void *), void* userarg);

static FASTFN void sel4bench_init() {
	//do kernel-mode PMC init
#ifndef CONFIG_EXPORT_PMU_USER
	seL4_DebugRun(&sel4bench_private_init, NULL);
#endif

	//ensure all counters are in the stopped state
	sel4bench_private_write_cntenc(-1);

	//Clear div 64 flag
	MODIFY_PMCR(&, ~SEL4BENCH_ARMV7A_PMCR_DIV64);

    //Reset all counters
	MODIFY_PMCR(|, SEL4BENCH_ARMV7A_PMCR_RESET_ALL | SEL4BENCH_ARMV7A_PMCR_RESET_CCNT);

	//Enable counters globally.
	MODIFY_PMCR(|, SEL4BENCH_ARMV7A_PMCR_ENABLE);

#ifdef ARM_HYP
	// Select instruction count incl. PL2 by default */
	sel4bench_private_write_pmnxsel(0x1f);
	sel4bench_private_write_evtsel(BIT(27));
#endif
	//start CCNT
	sel4bench_private_write_cntens(BIT(SEL4BENCH_ARMV7A_COUNTER_CCNT));
}

static FASTFN void sel4bench_destroy() {
	//stop all performance counters
	sel4bench_private_write_cntenc(-1);

	//Disable counters globally.
	MODIFY_PMCR(&, ~SEL4BENCH_ARMV7A_PMCR_ENABLE);

	//disable user-mode performance-counter access
#ifndef CONFIG_EXPORT_PMU_USER
	seL4_DebugRun(&sel4bench_private_deinit, NULL);
#endif
}

static FASTFN seL4_Word sel4bench_get_num_counters() {
#ifdef CORTEX_A8
	return 4;
#else //CORTEX_A8
	return SEL4BENCH_ARMV7A_PMCR_N(sel4bench_private_read_pmcr());
#endif //CORTEX_A8
}

static FASTFN sel4bench_counter_t sel4bench_get_cycle_count() {
	uint32_t enable_word = sel4bench_private_read_cntens(); //store running state

	sel4bench_private_write_cntenc(BIT(SEL4BENCH_ARMV7A_COUNTER_CCNT)); //stop CCNT
	uint32_t val = sel4bench_private_read_ccnt(); //read its value
	sel4bench_private_write_cntens(enable_word); //start it again if it was running

	return val;
}

/* being declared FASTFN allows this function (once inlined) to cache miss; I
 * think it's worthwhile in the general case, for performance reasons.
 * moreover, it's small enough that it'll be suitably aligned most of the time
 */
static FASTFN sel4bench_counter_t sel4bench_get_counter(seL4_Word counter) {
	sel4bench_private_write_pmnxsel(counter); //select the counter on the PMU

	counter = BIT(counter); //from here on in, we operate on a bitfield

	uint32_t enable_word = sel4bench_private_read_cntens();

	sel4bench_private_write_cntenc(counter); //stop the counter
	uint32_t val = sel4bench_private_read_pmcnt(); //read its value
	sel4bench_private_write_cntens(enable_word); //start it again if it was running

	return val;
}

/* this reader function is too complex to be inlined, so we force it to be
 * cacheline-aligned in order to avoid icache misses with the counters off.
 * (relevant note: GCC compiles this function to be exactly one ARMV7 cache
 * line in size) however, the pointer dereference is overwhelmingly likely to
 * produce a dcache miss, which will occur with the counters off
 */
static CACHESENSFN sel4bench_counter_t sel4bench_get_counters(seL4_Word counters, sel4bench_counter_t* values) {
	//we don't really have time for a NULL or bounds check here

	uint32_t enable_word = sel4bench_private_read_cntens(); //store current running state

	sel4bench_private_write_cntenc(enable_word); //stop running counters (we do this instead of stopping the ones we're interested in because it saves an instruction)

	unsigned int counter = 0;
	for(; counters != 0; counters >>= 1, counter++) { //for each counter...
		if(counters & 1) { //... if we care about it...
			sel4bench_private_write_pmnxsel(counter); //select it,
			values[counter] = sel4bench_private_read_pmcnt(); //and read its value
		}
	}

	uint32_t ccnt = sel4bench_private_read_ccnt(); //finally, read CCNT

	sel4bench_private_write_cntens(enable_word); //start the counters again

	return ccnt;
}

static FASTFN void sel4bench_set_count_event(seL4_Word counter, seL4_Word event) {
	sel4bench_private_write_pmnxsel(counter); //select counter
	sel4bench_private_write_pmcnt(0); //reset it
	return sel4bench_private_write_evtsel(event); //change the event
}

static FASTFN void sel4bench_start_counters(seL4_Word counters) {
	/* conveniently, ARM performance counters work exactly like this,
	 * so we just write the value directly to COUNTER_ENABLE_SET
	 */
	return sel4bench_private_write_cntens(counters);
}

static FASTFN void sel4bench_stop_counters(seL4_Word counters) {
	/* conveniently, ARM performance counters work exactly like this,
	 * so we just write the value directly to COUNTER_ENABLE_SET
	 * (protecting the CCNT)
	 */
	return sel4bench_private_write_cntenc(counters & ~BIT(SEL4BENCH_ARMV7A_COUNTER_CCNT));
}

static FASTFN void sel4bench_reset_counters(seL4_Word counters) {
    //Reset all counters except the CCNT
	if(counters == (~0UL) || counters == (BIT(sel4bench_get_num_counters()) - 1)) {
		MODIFY_PMCR(|, SEL4BENCH_ARMV7A_PMCR_RESET_ALL);
	} else {
		counters = counters & ~BIT(SEL4BENCH_ARMV7A_COUNTER_CCNT);
		uint32_t enable_word = sel4bench_private_read_cntens(); //store current running state

		sel4bench_private_write_cntenc(enable_word); //stop running counters (we do this instead of stopping the ones we're interested in because it saves an instruction)

		unsigned int counter = 0;
		for(; counters != 0; counters >>= 1, counter++) { //for each counter...
			if(counters & 1) { //... if we care about it...
				sel4bench_private_write_pmnxsel(counter); //select it,
				sel4bench_private_write_pmcnt(0);
			}
		}

		sel4bench_private_write_cntens(enable_word); //start the counters again
	}
}

#endif /* __ARCH_ARMV7A_SEL4BENCH_H__ */


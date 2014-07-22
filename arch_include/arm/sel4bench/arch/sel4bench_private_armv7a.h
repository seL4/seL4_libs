/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __SEL4BENCH_PRIVATE_H__
#define __SEL4BENCH_PRIVATE_H__

typedef uint32_t sel4bench_counter_t;
#define SEL4BENCH_COUNTER_FORMAT "u"

#include "sel4bench_events_armv7a.h"

//function attributes
//functions that need to be inlined for speed
#define FASTFN inline __attribute__((always_inline))
//functions that must not cache miss
#define CACHESENSFN __attribute__((noinline, aligned(64)))

//counters and related constants
#define SEL4BENCH_ARMV7A_NUM_COUNTERS 4

#define SEL4BENCH_ARMV7A_COUNTER_CCNT 31

// select whether user mode gets access to the PMCs
static FASTFN void sel4bench_private_switch_user_pmc(unsigned long state) {
	asm volatile (
			"mcr p15, 0, %0, c9, c14, 0\n"
			: /* no outputs */
			: "r" (state) /* input on/off state */
	);
}

/*
 * INTENS/INTENC
 *
 * determines if an interrupt is generated on overflow.
 */
static FASTFN void sel4bench_private_write_intens(uint32_t mask) {
	asm volatile (
		"mcr p15, 0, %0, c9, c14, 1\n"
		:
		: "r"(mask)
	);
}
static FASTFN void sel4bench_private_write_intenc(uint32_t mask) {
	asm volatile (
		"mcr p15, 0, %0, c9, c14, 2\n"
		:
		: "r"(mask)
		);
}

static void sel4bench_private_init(void* data) {
	//enable user-mode performance-counter access
	sel4bench_private_switch_user_pmc(1);

	//disable overflow interrupts on all counters
	sel4bench_private_write_intenc(-1);
}
static void sel4bench_private_deinit(void* data) {
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
#define SEL4BENCH_ARMV7A_PMCR_N(x)       (((x) & 0xFFFF) >> 11)
#define SEL4BENCH_ARMV7A_PMCR_ENABLE     (1 << 0)
#define SEL4BENCH_ARMV7A_PMCR_RESET_ALL  (1 << 1)
#define SEL4BENCH_ARMV7A_PMCR_RESET_CCNT (1 << 2)
#define SEL4BENCH_ARMV7A_PMCR_DIV64      (1 << 3) /* Should CCNT be divided by 64? */
static FASTFN void sel4bench_private_write_pmcr(uint32_t val) {
	asm volatile (
		"mcr p15, 0, %0, c9, c12, 0\n"
		:
		: "r"(val)
	);
}
static FASTFN uint32_t sel4bench_private_read_pmcr(void) {
	uint32_t val;
	asm volatile (
		"mrc p15, 0, %0, c9, c12, 0\n"
		: "=r"(val)
		:
	);
	return val;
}

/*
 * CNTENS/CNTENC (Count Enable Set/Clear)
 *
 * bit corresponds to which counter.
 */
static FASTFN void sel4bench_private_write_cntens(uint32_t mask) {
	asm volatile (
		"mcr p15, 0, %0, c9, c12, 1\n"
		:
		: "r"(mask)
	);
}
static FASTFN uint32_t sel4bench_private_read_cntens() {
	uint32_t mask;
	asm volatile (
		"mrc p15, 0, %0, c9, c12, 1\n"
		: "=r"(mask)
		:
	);
	return mask;
}
static FASTFN void sel4bench_private_write_cntenc(uint32_t mask) {
	asm volatile (
		"mcr p15, 0, %0, c9, c12, 2\n"
		:
		: "r"(mask)
	);
}

/*
 * PMCNT0-3: event count
 *
 * counter is selected by PMNXSEL register.
 */
static FASTFN uint32_t sel4bench_private_read_pmcnt(void) {
	uint32_t val;
	asm volatile (
		"mrc p15, 0, %0, c9, c13, 2\n"
		: "=r"(val)
		:
	);
	return val;
}
static FASTFN void sel4bench_private_write_pmcnt(uint32_t val) {
	asm volatile (
		"mcr p15, 0, %0, c9, c13, 2\n"
		: "=r"(val)
		:
	);
}


/*
 * PMNXSEL
 *
 * selects which counter is used by get_pmnct.
 */
static FASTFN void sel4bench_private_write_pmnxsel(uint32_t val) {
	asm volatile (
		"mcr p15, 0, %0, c9, c12, 5\n"
		:
		: "r"(val)
	);
}

/*
 * CCNT
 * 
 * the cycle counter.
 */
static FASTFN uint32_t sel4bench_private_read_ccnt() {
	uint32_t val;
	asm volatile (
		"mrc p15, 0, %0, c9, c13, 0\n"
		: "=r"(val)
	);
	return val;
}

/*
 * EVTSEL
 *
 * determines which events are counted by a performance counter.
 * counter is selected by PMNXSEL register.
 */
static FASTFN uint32_t sel4bench_private_read_evtsel(void) {
	uint32_t val;
	asm volatile (
		"mrc p15, 0, %0, c9, c13, 1\n"
		: "=r"(val)
		:
	);
	return val;
}

static FASTFN void sel4bench_private_write_evtsel(uint32_t val) {
	asm volatile (
		"mcr p15, 0, %0, c9, c13, 1\n"
		:
		: "r"(val)
	);
}

#endif /* __SEL4BENCH_PRIVATE_H__ */

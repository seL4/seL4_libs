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

#include <stdint.h>

typedef uint64_t sel4bench_counter_t;
#define SEL4BENCH_COUNTER_FORMAT "llu"

//function attributes
//ultra-short, time-sensitive functions
#define FASTFN inline __attribute__((always_inline))

//short, cache-sensitive functions (note: short means one cache line)
#define CACHESENSFN __attribute__((noinline, aligned(64)))

//functions that will be passed to seL4_DebugRun() -- fast, but obviously not inlined
#define KERNELFN __attribute__((noinline, flatten))

#ifndef BIT
#define BIT(x) (1ul << (x))
#endif

#define IN_TX_BIT BIT(0)
#define IN_TXCP_BIT BIT(1)

#include "events.h"

//CPUID leaf node numbers
enum {
	IA32_CPUID_LEAF_BASIC    = 0,
	IA32_CPUID_LEAF_MODEL    = 1,
	IA32_CPUID_LEAF_PMC      = 0xA,
	IA32_CPUID_LEAF_EXTENDED = 0x80000000,
};

//CPUID.0 "GenuineIntel"
#define IA32_CPUID_BASIC_MAGIC_EBX 0x756E6547
#define IA32_CPUID_BASIC_MAGIC_ECX 0x6C65746E
#define IA32_CPUID_BASIC_MAGIC_EDX 0x49656E69

//CPUID.1 Family and Model ID macros and type.
#define FAMILY(x) (  (x).family == 0xF                       ? ( (x).ex_family      + (x).family) : (x).family )
#define MODEL(x)  ( ((x).family == 0xF || (x).family == 0x6) ? (((x).ex_model << 4) + (x).model ) : (x).model  )
#define IA32_CPUID_FAMILY_P6 0x6
#define IA32_CPUID_MODEL_MIN_CORE     0x0E
#define IA32_CPUID_MODEL_MIN_CORE2    0x0F
#define IA32_CPUID_MODEL_MIN_NEHALEM  0x1A
#define IA32_CPUID_MODEL_MIN_WESTMERE 0x2F
#define IA32_CPUID_MODEL_MIN_HASWELL  0x3A
typedef union {
	struct {
		seL4_Word stepping  : 4;
		seL4_Word model     : 4;
		seL4_Word family    : 4;
		seL4_Word type      : 2;
		seL4_Word reserved1 : 2;
		seL4_Word ex_model  : 4;
		seL4_Word ex_family : 8;
		seL4_Word reserved2 : 4;
	};
	seL4_Word raw;
} ia32_cpuid_model_info_t;

//CPUID.PMC Performance-monitoring macros and types
typedef union {
	struct {
		uint8_t pmc_version_id;
		uint8_t gp_pmc_count_per_core;
		uint8_t gp_pmc_bit_width;
		uint8_t ebx_bit_vector_length;
	};
    uint32_t  raw;
} ia32_cpuid_leaf_pmc_eax_t;


//Control-register constants
#define IA32_CR4_PCE               8


//PMC MSRs
#define IA32_MSR_PMC_PERFEVTSEL_BASE 0x186
#define IA32_MSR_PMC_PERFEVTCNT_BASE 0x0C1
typedef union {
	struct {
		uint16_t event;
		union {
			struct {
				uint8_t USR : 1;
				uint8_t OS  : 1;
				uint8_t E   : 1;
				uint8_t PC  : 1;
				uint8_t INT : 1;
				uint8_t res : 1;
				uint8_t EN  : 1;
				uint8_t INV : 1;
			};
			uint8_t flags;
		};
		uint8_t cmask;
	};
	seL4_Word raw;
} ia32_pmc_perfevtsel_t;


//Convenient execution of CPUID instruction. The first version isn't volatile, so is for querying the processor; the second version just serialises.
//This looks slow, but gcc inlining is smart enough to optimise away all the memory references, and takes unused information into account.
static FASTFN void sel4bench_private_cpuid(seL4_Word leaf, seL4_Word subleaf, seL4_Word * eax, seL4_Word * ebx, seL4_Word * ecx, seL4_Word * edx) {
	asm (
		"cpuid"
		: "=a"(*eax)    /* output eax */
		, "=b"(*ebx)    /* output ebx */
		, "=c"(*ecx)    /* output ecx */
		, "=d"(*edx)    /* output edx */
		: "a" (leaf)    /* input query leaf */
		, "c" (subleaf) /* input query subleaf */
	);
}
static FASTFN void sel4bench_private_cpuid_serial() {
	//set leaf and subleaf to 0 for predictability
	uint32_t eax = 0;
	uint32_t ecx = 0;
	asm volatile (
		"cpuid"
		: "+a"(eax)       /* eax = 0 and gets clobbered */
		, "+c"(ecx)       /* ecx = 0 and gets clobbered */
		:                 /* no other inputs to this version */
		: "%ebx"          /* clobber ebx */
		, "%edx"          /* clobber edx */
		, "cc"            /* clobber condition code */
	);
}
static FASTFN void sel4bench_private_lfence() {
	asm volatile("lfence");
}


static FASTFN uint64_t sel4bench_private_rdtsc() {
	uint64_t time;
#ifdef X86_64
    uint64_t lo, hi;
    asm volatile (
            "rdtsc"
            : "=a"(lo), "=d"(hi)
            );
    return ((hi << 32) | lo);
#else
	asm volatile (
		"rdtsc"      /* Get the cycle count */
		: "=A"(time) /* output value (edx:eax) */
	);
#endif
	return time;
}

static FASTFN uint64_t sel4bench_private_rdpmc(uint32_t counter) {
#ifdef X86_64
    uint64_t hi, lo;
    asm volatile (
            "rdpmc"
            : "=a"(lo), "=d"(hi)
            : "c"(counter)
            );
    return ((hi << 32 ) | lo);
#else
	uint64_t counter_val;
	asm volatile (
		"rdpmc"             /* Read the performance counter */
		: "=A"(counter_val) /* output value (edx:eax) */
		: "c" (counter)     /* input counter (low 29 bits) */
	);
	return counter_val;
#endif
}

//Serialization instruction for before and after reading PMCs
//See comment in arch/sel4bench.h for details.
#ifdef SEL4BENCH_STRICT_PMC_SERIALIZATION
#define sel4bench_private_serialize_pmc sel4bench_private_cpuid_serial
#else //SEL4BENCH_STRICT_PMC_SERIALIZATION
#define sel4bench_private_serialize_pmc sel4bench_private_lfence 
#endif //SEL4BENCH_STRICT_PMC_SERIALIZATION

//enable user-level pmc access
static KERNELFN void sel4bench_private_enable_user_pmc(void* arg) {
#ifdef X86_64

    uint64_t dummy;
    asm volatile (
        "movq   %%cr4, %0;"
        "orq    %[pce], %0;"
        "movq   %0, %%cr4;"
        : "=r" (dummy)
        : [pce] "i" BIT(IA32_CR4_PCE)
        : "cc"
    );
#else

	uint32_t dummy;
	asm volatile (
		"movl %%cr4, %0;"               /* read CR4 */
		"orl %[pce], %0;"               /* enable PCE flag */
		"movl %0, %%cr4;"               /* write CR4 */
		: "=r" (dummy)                  /* fake output to ask GCC for a register */
		: [pce] "i" BIT(IA32_CR4_PCE) /* input PCE flag */
		: "cc"                          /* clobber condition code */
	);
#endif
}

//disable user-level pmc access
static KERNELFN void sel4bench_private_disable_user_pmc(void* arg) {
#ifdef X86_64
    uint64_t dummy;
    asm volatile (
        "movq   %%cr4, %0;"
        "andq   %[pce], %0;"
        "movq   %0, %%cr4;"
        : "=r" (dummy)
        : [pce] "i" (~BIT(IA32_CR4_PCE))
        : "cc"
    );

#else
	uint32_t dummy;
	asm volatile (
		"movl %%cr4, %0;"                  /* read CR4 */
		"andl %[pce], %0;"                 /* enable PCE flag */
		"movl %0, %%cr4;"                  /* write CR4 */
		: "=r" (dummy)                     /* fake output to ask GCC for a register */
		: [pce] "i" (~BIT(IA32_CR4_PCE)) /* input PCE flag */
		: "cc"                             /* clobber condition code */
	);
#endif
}

//read an MSR
static KERNELFN void sel4bench_private_rdmsr(void* arg) {
	seL4_Word* msr = (seL4_Word*)arg;

	asm volatile (
		"rdmsr"
		: "=a" (msr[1]) /* output low */
		, "=d" (msr[2]) /* output high */
		:  "c" (msr[0]) /* input MSR index */
	);
}

//write an MSR
static KERNELFN void sel4bench_private_wrmsr(void* arg) {
	seL4_Word* msr = (seL4_Word*)arg;

	asm volatile (
		"wrmsr"
		:               /* no output */
		: "a" (msr[1])  /* input low */
		, "d" (msr[2])  /* input high */
		, "c" (msr[0])  /* input MSR index */
	);
}

//generic event tables for lookup fn below
//they use direct event numbers, rather than the constants in events.h, because it's smaller
static seL4_Word SEL4BENCH_IA32_WESTMERE_EVENTS[5] = {
	0x0280, //CACHE_L1I_MISS
	0x0151, //CACHE_L1D_MISS, must use counter 0 or 1
	0x20C8, //TLB_L1I_MISS
	0x80CB, //TLB_L1D_MISS
	0x5FCB  //SEL4BENCH_IA32_WESTMERE_EVENT_CACHE_|{L1D_HIT,L2_HIT,L3P_HIT,L3_HIT,L3_MISS,LFB_HIT}_R
};
static seL4_Word SEL4BENCH_IA32_NEHALEM_EVENTS[5] = {
	0x0280, //CACHE_L1I_MISS
	0x0151, //CACHE_L1D_MISS, must use counter 0 or 1
	0x20C8, //TLB_L1I_MISS
	0x80CB, //TLB_L1D_MISS
	0x5FCB  //SEL4BENCH_IA32_NEHALEM_EVENT_CACHE_|{L1D_HIT,L2_HIT,L3P_HIT,L3_HIT,L3_MISS,LFB_HIT}_R
};
static seL4_Word SEL4BENCH_IA32_CORE2_EVENTS[5] = {
	0x0081, //CACHE_L1I_MISS
	0x01CB, //CACHE_L1D_MISS, must use counter 0
	0x00C9, //TLB_L1I_MISS
	0x10CB, //TLB_L1D_MISS, must use counter 0
	0x03C0  //SEL4BENCH_IA32_CORE2_EVENT_RETIRE_MEMORY_|{READ,WRITE}
};
static seL4_Word SEL4BENCH_IA32_CORE_EVENTS[5] = {
	0x0081, //CACHE_L1I_MISS
	0x0000, //CACHE_L1D_MISS, not available on CORE
	0x0085, //TLB_L1I_MISS
	0x0049, //TLB_L1D_MISS
	0x0143  //MEMORY_ACCESS
};
static seL4_Word SEL4BENCH_IA32_P6_EVENTS[5] = {
	0x0081, //CACHE_L1I_MISS
	0x0045, //CACHE_L1D_MISS
	0x0085, //TLB_L1I_MISS
	0x0000, //TLB_L1D_MISS, not available on P6
	0x0043  //MEMORY_ACCESS
};
static seL4_Word SEL4BENCH_IA32_HASWELL_EVENTS[5] = {
	0x0280, //CACHE_L1I_MISS
	0x0151, //CACHE_L1D_MISS, must use counter 0 or 1
	0x0085, //TLB_L1I_MISS
	0x0049, //TLB_L1D_MISS
	0x412E  //LLC_MISS
};

static FASTFN seL4_Word sel4bench_private_lookup_event(seL4_Word event) {
	if(SEL4BENCH_IA32_EVENT_GENERIC_MASK & event) {
		seL4_Word dummy = 0;
		ia32_cpuid_model_info_t model_info = { .raw = 0 };
		sel4bench_private_cpuid(IA32_CPUID_LEAF_MODEL, 0, &model_info.raw, &dummy, &dummy, &dummy);

		//we should be a P6
		assert(FAMILY(model_info) == IA32_CPUID_FAMILY_P6);

		event =
			event & ~SEL4BENCH_IA32_EVENT_GENERIC_MASK;

		//P3 or PM
		if(MODEL(model_info) < IA32_CPUID_MODEL_MIN_CORE)
			return SEL4BENCH_IA32_P6_EVENTS[event];

		//CORE
		if(MODEL(model_info) < IA32_CPUID_MODEL_MIN_CORE2)
			return SEL4BENCH_IA32_CORE_EVENTS[event];

		//CORE2
		if(MODEL(model_info) < IA32_CPUID_MODEL_MIN_NEHALEM)
			return SEL4BENCH_IA32_CORE2_EVENTS[event];

		//NEHALEM
		if(MODEL(model_info) < IA32_CPUID_MODEL_MIN_WESTMERE)
			return SEL4BENCH_IA32_NEHALEM_EVENTS[event];

		//WESTMERE
        if(MODEL(model_info) < IA32_CPUID_MODEL_MIN_HASWELL) 
            return SEL4BENCH_IA32_WESTMERE_EVENTS[event];

		//HASWELL
		return SEL4BENCH_IA32_HASWELL_EVENTS[event];
	} else {
		return event;
	}
}

#endif /* __SEL4BENCH_PRIVATE_H__ */

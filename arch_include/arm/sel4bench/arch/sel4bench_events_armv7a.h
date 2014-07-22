/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

//event definitions
//events common to all ARMV7A CPUs
#define SEL4BENCH_ARMV7A_EVENT_SOFTWARE_INCREMENT          0x00
#define SEL4BENCH_ARMV7A_EVENT_CACHE_L1I_MISS              0x01
#define SEL4BENCH_ARMV7A_EVENT_TLB_L1I_MISS                0x02
#define SEL4BENCH_ARMV7A_EVENT_CACHE_L1D_MISS              0x03
#define SEL4BENCH_ARMV7A_EVENT_CACHE_L1D_HIT               0x04
#define SEL4BENCH_ARMV7A_EVENT_TLB_L1D_MISS                0x05
#define SEL4BENCH_ARMV7A_EVENT_MEMORY_READ                 0x06
#define SEL4BENCH_ARMV7A_EVENT_MEMORY_WRITE                0x07
#define SEL4BENCH_ARMV7A_EVENT_EXECUTE_INSTRUCTION         0x08
#define SEL4BENCH_ARMV7A_EVENT_EXCEPTION                   0x09
#define SEL4BENCH_ARMV7A_EVENT_EXCEPTION_RETURN            0x0A
#define SEL4BENCH_ARMV7A_EVENT_CONTEXTIDR_WRITE            0x0B
#define SEL4BENCH_ARMV7A_EVENT_SOFTWARE_PC_CHANGE          0x0C
#define SEL4BENCH_ARMV7A_EVENT_EXECUTE_BRANCH_IMM          0x0D
#define SEL4BENCH_ARMV7A_EVENT_FUNCTION_RETURN             0x0E
#define SEL4BENCH_ARMV7A_EVENT_MEMORY_ACCESS_UNALIGNED     0x0F
#define SEL4BENCH_ARMV7A_EVENT_BRANCH_MISPREDICT           0x10
#define SEL4BENCH_ARMV7A_EVENT_CCNT                        0x11
#define SEL4BENCH_ARMV7A_EVENT_EXECUTE_BRANCH_PREDICTABLE  0x12

#ifdef CORTEX_A8
#include "sel4bench_events_cortexa8.h"
#endif //CORTEX_A8

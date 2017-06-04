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
//event definitions

/* generic events */
#define SEL4BENCH_EVENT_CACHE_L1I_MISS          0x00
#define SEL4BENCH_EVENT_CACHE_L1D_MISS          0x0B
#define SEL4BENCH_EVENT_TLB_L1I_MISS            0x03
#define SEL4BENCH_EVENT_TLB_L1D_MISS            0x04
#define SEL4BENCH_EVENT_MEMORY_ACCESS           0x10
#define SEL4BENCH_EVENT_EXECUTE_INSTRUCTION     0x07
#define SEL4BENCH_EVENT_BRANCH_MISPREDICT       0x06

/* specific events */
#define SEL4BENCH_EVENT_STALL_INSTRUCTION       0x01
#define SEL4BENCH_EVENT_STALL_DATA              0x02
#define SEL4BENCH_EVENT_EXECUTE_BRANCH          0x05
#define SEL4BENCH_EVENT_CACHE_L1D_HIT           0x09
#define SEL4BENCH_EVENT_CACHE_L1D_ACCESS        0x0A
#define SEL4BENCH_EVENT_CACHE_L1D_WRITEBACK_HL  0x0C
#define SEL4BENCH_EVENT_SOFTWARE_PC_CHANGE      0x0D
#define SEL4BENCH_EVENT_TLB_L2_MISS             0x0F
#define SEL4BENCH_EVENT_STALL_LSU_BUSY          0x11
#define SEL4BENCH_EVENT_WRITE_BUFFER_DRAIN      0x12
#define SEL4BENCH_EVENT_ETMEXTOUT_0             0x20
#define SEL4BENCH_EVENT_ETMEXTOUT_1             0x21
#define SEL4BENCH_EVENT_ETMEXTOUT               0x22
#define SEL4BENCH_EVENT_CCNT                    0xFF

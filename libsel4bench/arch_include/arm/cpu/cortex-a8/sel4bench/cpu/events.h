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
//events specific to the Cortex-A8
#define SEL4BENCH_EVENT_WRITE_BUFFER_DRAIN       0x40
#define SEL4BENCH_EVENT_CACHE_L2_WRITE           0x41
#define SEL4BENCH_EVENT_CACHE_L2_STORE           0x42
#define SEL4BENCH_EVENT_CACHE_L2_READ            0x43
#define SEL4BENCH_EVENT_CACHE_L2_MISS            0x44
#define SEL4BENCH_EVENT_AXI_READ                 0x45
#define SEL4BENCH_EVENT_AXI_WRITE                0x46
#define SEL4BENCH_EVENT_MEMORY_REPLAY            0x47
#define SEL4BENCH_EVENT_MEMORY_UNALIGNED_REPLAY  0x48
#define SEL4BENCH_EVENT_CACHE_L1D_MISS_HASH      0x49
#define SEL4BENCH_EVENT_CACHE_L1I_MISS_HASH      0x4A
#define SEL4BENCH_EVENT_CACHE_L1D_CONFLICT_EVICT 0x4B
#define SEL4BENCH_EVENT_CACHE_L1D_HIT_NEON       0x4C
#define SEL4BENCH_EVENT_CACHE_L1D_ACCESS_NEON    0x4D
#define SEL4BENCH_EVENT_CACHE_L2_ACCESS_NEON     0x4E
#define SEL4BENCH_EVENT_CACHE_L2_HIT_NEON        0x4F
#define SEL4BENCH_EVENT_CACHE_L1I_ACCESS         0x50
#define SEL4BENCH_EVENT_RETURN_STACK_MISPREDICT  0x51
#define SEL4BENCH_EVENT_BRANCH_MISPREDICT        0x52
#define SEL4BENCH_EVENT_BRANCH_PREDICT_TRUE      0x53
#define SEL4BENCH_EVENT_EXECUTE_BRANCH_TRUE      0x54
#define SEL4BENCH_EVENT_EXECUTE_OPERATION        0x55
#define SEL4BENCH_EVENT_STALL_INSTRUCTION        0x56
#define SEL4BENCH_EVENT_IPC                      0x57
#define SEL4BENCH_EVENT_STALL_NEON_DATA          0x58
#define SEL4BENCH_EVENT_STALL_NEON_EXECUTION     0x59
#define SEL4BENCH_EVENT_PROCESSOR_TOTALLY_BUSY   0x5A
#define SEL4BENCH_EVENT_PMUEXTIN_0               0x70
#define SEL4BENCH_EVENT_PMUEXTIN_1               0x71
#define SEL4BENCH_EVENT_PMUEXTIN                 0x72

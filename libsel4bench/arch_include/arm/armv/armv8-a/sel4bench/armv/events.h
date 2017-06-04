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

#include <autoconf.h>

/* generic events */
#define SEL4BENCH_EVENT_CACHE_L1I_MISS              0x01
#define SEL4BENCH_EVENT_CACHE_L1D_MISS              0x03
#define SEL4BENCH_EVENT_TLB_L1I_MISS                0x02
#define SEL4BENCH_EVENT_TLB_L1D_MISS                0x05
#define SEL4BENCH_EVENT_EXECUTE_INSTRUCTION         0x08
#define SEL4BENCH_EVENT_BRANCH_MISPREDICT           0x10

/* further events */
#define SEL4BENCH_EVENT_SOFTWARE_INCREMENT          0x00
#define SEL4BENCH_EVENT_CACHE_L1D_HIT               0x04
#define SEL4BENCH_EVENT_MEMORY_READ                 0x06
#define SEL4BENCH_EVENT_MEMORY_WRITE                0x07
#define SEL4BENCH_EVENT_EXCEPTION                   0x09
#define SEL4BENCH_EVENT_EXCEPTION_RETURN            0x0A
#define SEL4BENCH_EVENT_CONTEXTIDR_WRITE            0x0B
#define SEL4BENCH_EVENT_SOFTWARE_PC_CHANGE          0x0C
#define SEL4BENCH_EVENT_EXECUTE_BRANCH_IMM          0x0D
#define SEL4BENCH_EVENT_FUNCTION_RETURN             0x0E
#define SEL4BENCH_EVENT_MEMORY_ACCESS_UNALIGNED     0x0F
#define SEL4BENCH_EVENT_CCNT                        0x11
#define SEL4BENCH_EVENT_EXECUTE_BRANCH_PREDICTABLE  0x12
/* Data memory access */
#define SEL4BENCH_EVENT_MEMORY_ACCESS                  0x13
/* Level 1 instruction cache access */
#define SEL4BENCH_EVENT_L1I_CACHE                   0x14
/* Level 1 data cache write-back */
#define SEL4BENCH_EVENT_L1D_CACHE_WB                0x15
/* Level 2 data cache access */
#define SEL4BENCH_EVENT_L2D_CACHE                   0x16
/* Level 2 data cache refill */
#define SEL4BENCH_EVENT_L2D_CACHE_REFILL            0x17
/* Level 2 data cache write-back */
#define SEL4BENCH_EVENT_L2D_CACHE_WB                0x18
/* Bus access */
#define SEL4BENCH_EVENT_BUS_ACCESS                  0x19
/* Local memory error */
#define SEL4BENCH_EVENT_MEMORY_ERROR                0x1A
/* Instruction speculatively executed */
#define SEL4BENCH_EVENT_INST_SPEC                   0x1B
/* Instruction architecturally executed, condition code check pass, write to TTBR */
#define SEL4BENCH_EVENT_TTBR_WRITE_RETIRED          0x1C
/* Bus cycle */
#define SEL4BENCH_EVENT_BUS_CYCLES                  0x1D
#define SEL4BENCH_EVENT_CHAIN                       0x1E
#define SEL4BENCH_EVENT_L1D_CACHE_ALLOCATE          0x1F
#define SEL4BENCH_EVENT_L2D_CACHE_ALLOCATE          0x20
#define SEL4BENCH_EVENT_BR_RETIRED                  0x21
#define SEL4BENCH_EVENT_BR_MIS_PRED_RETIRED         0x22
#define SEL4BENCH_EVENT_STALL_FRONTEND              0x23
#define SEL4BENCH_EVENT_STALL_BACKEND               0x24
#define SEL4BENCH_EVENT_L1D_TLB                     0x25
#define SEL4BENCH_EVENT_L1I_TLB                     0x26
#define SEL4BENCH_EVENT_L2I_CACHE                   0x27
#define SEL4BENCH_EVENT_L2I_CACHE_REFILL            0x28
#define SEL4BENCH_EVENT_L3D_CACHE_ALLOCATE          0x29
#define SEL4BENCH_EVENT_L3D_CACHE_REFILL            0x2A
#define SEL4BENCH_EVENT_L3D_CACHE                   0x2B
#define SEL4BENCH_EVENT_L3D_CACHE_WB                0x2C
#define SEL4BENCH_EVENT_L2D_TLB_REFILL              0x2D
#define SEL4BENCH_EVENT_L2I_TLB_REFILL              0x2E
#define SEL4BENCH_EVENT_L2D_TLB                     0x2F
#define SEL4BENCH_EVENT_L2I_TLB                     0x30

#include <sel4bench/cpu/events.h>

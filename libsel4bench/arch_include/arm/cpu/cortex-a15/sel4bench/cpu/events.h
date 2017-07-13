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
/*
 * event definitions
 * events specific to the Cortex-A15
 */

#define SEL4BENCH_EVENT_L1D_CACHE_LD          0x40 /* Level 1 data cache access, read */
#define SEL4BENCH_EVENT_L1D_CACHE_ST          0x41 /* Level 1 data cache access, write */
#define SEL4BENCH_EVENT_L1D_CACHE_REFILL_LD   0x42 /* Level 1 data cache refill, read */
#define SEL4BENCH_EVENT_L1D_CACHE_REFILL_ST   0x43 /* Level 1 data cache refill, write */
#define SEL4BENCH_EVENT_L1D_CACHE_WB_VICTIM   0x46 /* Level 1 data cache write-back, victim */
#define SEL4BENCH_EVENT_L1D_CACHE_WB_CLEAN    0x47 /* Level 1 data cache write-back, cleaning and coherency */
#define SEL4BENCH_EVENT_L1D_CACHE_INVAL       0x48 /* Level 1 data cache invalidate */
#define SEL4BENCH_EVENT_L1D_TLB_REFILL_LD     0x4C /* Level 1 data TLB refill, read */
#define SEL4BENCH_EVENT_L1D_TLB_REFILL_ST     0x4D /* Level 1 data TLB refill, write */
#define SEL4BENCH_EVENT_L2D_CACHE_LD          0x50 /* Level 2 data cache access, read */
#define SEL4BENCH_EVENT_L2D_CACHE_ST          0x51 /* Level 2 data cache access, write */
#define SEL4BENCH_EVENT_L2D_CACHE_REFILL_LD   0x52 /* Level 2 data cache refill, read */
#define SEL4BENCH_EVENT_L2D_CACHE_REFILL_ST   0x53 /* Level 2 data cache refill, write */
#define SEL4BENCH_EVENT_L2D_CACHE_WB_VICTIM   0x56 /* Level 2 data cache write-back, victim */
#define SEL4BENCH_EVENT_L2D_CACHE_WB_CLEAN    0x57 /* Level 2 data cache write-back, cleaning and coherency */
#define SEL4BENCH_EVENT_L2D_CACHE_INVAL       0x58 /* Level 2 data cache invalidate */
#define SEL4BENCH_EVENT_BUS_ACCESS_LD         0x60 /* Bus access, read */
#define SEL4BENCH_EVENT_BUS_ACCESS_ST         0x61 /* Bus access, write */
#define SEL4BENCH_EVENT_BUS_ACCESS_SHARED     0x62 /* Bus access, Normal, Cacheable, Shareable */
#define SEL4BENCH_EVENT_BUS_ACCESS_NOT_SHARED 0x63 /* Bus access, not Normal, Cacheable, Shareable */
#define SEL4BENCH_EVENT_BUS_ACCESS_NORMAL     0x64 /* Bus access, normal */
#define SEL4BENCH_EVENT_BUS_ACCESS_PERIPH     0x65 /* Bus access, peripheral */
#define SEL4BENCH_EVENT_MEM_ACCESS_LD         0x66 /* Data memory access, read */
#define SEL4BENCH_EVENT_MEM_ACCESS_ST         0x67 /* Data memory access, write */
#define SEL4BENCH_EVENT_UNALIGNED_LD_SPEC     0x68 /* Unaligned access, read */
#define SEL4BENCH_EVENT_UNALIGNED_ST_SPEC     0x69 /* Unaligned access, write */
#define SEL4BENCH_EVENT_UNALIGNED_LDST_SPEC   0x6A /* Unaligned access */
#define SEL4BENCH_EVENT_LDREX_SPEC            0x6C /* Exclusive instruction speculatively executed, LDREX */
#define SEL4BENCH_EVENT_STREX_PASS_SPEC       0x6D /* Exclusive instruction speculatively executed, STREX pass */
#define SEL4BENCH_EVENT_STREX_FAIL_SPEC       0x6E /* Exclusive instruction speculatively executed, STREX fail */
#define SEL4BENCH_EVENT_LD_SPEC               0x70 /* Instruction speculatively executed, load */
#define SEL4BENCH_EVENT_ST_SPEC               0x71 /* Instruction speculatively executed, store */
#define SEL4BENCH_EVENT_LDST_SPEC             0x72 /* Instruction speculatively executed, load or store */
#define SEL4BENCH_EVENT_DP_SPEC               0x73 /* Instruction speculatively executed, integer data processing */
#define SEL4BENCH_EVENT_ASE_SPEC              0x74 /* Instruction speculatively executed, Advanced SIMD Extension */
#define SEL4BENCH_EVENT_VFP_SPEC              0x75 /* Instruction speculatively executed, Floating-point Extension */
#define SEL4BENCH_EVENT_PC_WRITE_SPEC         0x76 /* Instruction speculatively executed, software change of the PC */
#define SEL4BENCH_EVENT_BR_IMMED_SPEC         0x78 /* Branch speculatively executed, immediate branch */
#define SEL4BENCH_EVENT_BR_RETURN_SPEC        0x79 /* Branch speculatively executed, procedure return */
#define SEL4BENCH_EVENT_BR_INDIRECT_SPEC      0x7A /* Branch speculatively executed, indirect branch */
#define SEL4BENCH_EVENT_ISB_SPEC              0x7C /* Barrier speculatively executed, ISB */
#define SEL4BENCH_EVENT_DSB_SPEC              0x7D /* Barrier speculatively executed, DSB */
#define SEL4BENCH_EVENT_DMB_SPEC              0x7E /* Barrier speculatively executed, DMB */

/*
 * ARM Morello performance monitoring events
 *
 * Copyright 2024, Capabilities Ltd <heshamalmatary@capabilitieslimited.co.uk>.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

// Event numbers for Morello CPU based on Neoverse N1 TRM Section C-2.3.
#define SEL4BENCH_EVENT_REMOTE_ACCESS          0x31 /* Access to another socket in a multi-socket system */
#define SEL4BENCH_EVENT_DTLB_WALK              0x34 /* Access to data TLB that caused a page table walk */
#define SEL4BENCH_EVENT_ITLB_WALK              0x35 /* Access to instruction TLB that caused a page table walk */
#define SEL4BENCH_EVENT_LL_CACHE_RD            0x36 /* Last level cache access, read. */
#define SEL4BENCH_EVENT_LL_CACHE_MISS_RD       0x37 /* Last level cache miss, read */
#define SEL4BENCH_EVENT_L1D_CACHE_RD           0x40 /* L1 data cache access, read */
#define SEL4BENCH_EVENT_L1D_CACHE_WR           0x41 /* L1 data cache access, write */
#define SEL4BENCH_EVENT_L1D_CACHE_REFILL_RD    0x42 /* L1 data cache refill, read */
#define SEL4BENCH_EVENT_L1D_CACHE_REFILL_WR    0x43 /* L1 data cache refill, write */
#define SEL4BENCH_EVENT_L1D_CACHE_REFILL_INNER 0x44 /* L1 data cache refill, inner */
#define SEL4BENCH_EVENT_L1D_CACHE_REFILL_OUTER 0x45 /* L1 data cache refill, outer */
#define SEL4BENCH_EVENT_L1D_CACHE_WB_VICTIM    0x46 /* L1 data cache write-back, victim */
#define SEL4BENCH_EVENT_L1D_CACHE_WB_CLEAN     0x47 /* L1 data cache write-back cleaning and coherency */
#define SEL4BENCH_EVENT_L1D_CACHE_INVAL        0x48 /* L1 data cache invalidate */
#define SEL4BENCH_EVENT_L1D_TLB_REFILL_RD      0x4c /* L1 data TLB refill, read */
#define SEL4BENCH_EVENT_L1D_TLB_REFILL_WR      0x4d /* L1 data TLB refill, write */
#define SEL4BENCH_EVENT_L1D_TLB_RD             0x4e /* L1 data TLB access, read */
#define SEL4BENCH_EVENT_L1D_TLB_WR             0x4f /* L1 data TLB access, write */
#define SEL4BENCH_EVENT_L2D_CACHE_RD           0x50 /* L2 data cache access, read  */
#define SEL4BENCH_EVENT_L2D_CACHE_WR           0x51 /* L2 data cache access, write  */
#define SEL4BENCH_EVENT_L2D_CACHE_REFILL_RD    0x52 /* L2 data cache refill, read */
#define SEL4BENCH_EVENT_L2D_CACHE_REFILL_WR    0x53 /* L2 data cache refill, write */
#define SEL4BENCH_EVENT_L2D_CACHE_WB_VICTIM    0x56 /* L2 data cache write-back, victim */
#define SEL4BENCH_EVENT_L2D_CACHE_WB_CLEAN     0x57 /* L2 data cache write-back, cleaning and coherency */
#define SEL4BENCH_EVENT_L2D_CACHE_INVAL        0x58 /* L2 data cache invalidate */
#define SEL4BENCH_EVENT_L2D_TLB_REFILL_RD      0x5c /* L2 data or unified TLB refill, read */
#define SEL4BENCH_EVENT_L2D_TLB_REFILL_WR      0x5d /* L2 data or unified TLB refill, write */
#define SEL4BENCH_EVENT_L2D_TLB_RD             0x5e /* L2 data or unified TLB access, read */
#define SEL4BENCH_EVENT_L2D_TLB_WR             0x5f /* L2 data or unified TLB access, write */
#define SEL4BENCH_EVENT_BUS_ACCESS_RD          0x60 /* Bus access read */
#define SEL4BENCH_EVENT_BUS_ACCESS_WR          0x61 /* Bus access write */
#define SEL4BENCH_EVENT_MEM_ACCESS_RD          0x66 /* Data memory access, read */
#define SEL4BENCH_EVENT_MEM_ACCESS_WR          0x67 /* Data memory access, write */
#define SEL4BENCH_EVENT_UNALIGNED_LD_SPEC      0x68 /* Unaligned access, read */
#define SEL4BENCH_EVENT_UNALIGNED_ST_SPEC      0x69 /* Unaligned access, write */
#define SEL4BENCH_EVENT_UNALIGNED_LDST_SPEC    0x6a /* Unaligned access */
#define SEL4BENCH_EVENT_LDREX_SPEC             0x6c /* Exclusive operation speculatively executed, LDREX or LDX */
#define SEL4BENCH_EVENT_STREX_PASS_SPEC        0x6d /* Exclusive operation speculatively executed, STREX or STX pass */
#define SEL4BENCH_EVENT_STREX_FAIL_SPEC        0x6e /* Exclusive operation speculatively executed, STREX or STX fail */
#define SEL4BENCH_EVENT_STREX_SPEC             0x6f /* Exclusive operation speculatively executed, STREX or STX */
#define SEL4BENCH_EVENT_LD_SPEC                0x70 /* Operation speculatively executed, load */
#define SEL4BENCH_EVENT_ST_SPEC                0x71 /* Operation speculatively executed, store */
#define SEL4BENCH_EVENT_LDST_SPEC              0x72 /* Operation speculatively executed, load or store */
#define SEL4BENCH_EVENT_DP_SPEC                0x73 /* Operation speculatively executed, integer data-processing */
#define SEL4BENCH_EVENT_ASE_SPEC               0x74 /* Operation speculatively executed, Advanced SIMD instruction */
#define SEL4BENCH_EVENT_VFP_SPEC               0x75 /* Operation speculatively executed, floating-point instruction */
#define SEL4BENCH_EVENT_PC_WRITE_SPEC          0x76 /* Operation speculatively executed, software change of the PC  */
#define SEL4BENCH_EVENT_CRYPTO_SPEC            0x77 /* Operation speculatively executed, Cryptographic instruction */
#define SEL4BENCH_EVENT_BR_IMMED_SPEC          0x78 /* Branch speculatively executed, immediate branch */
#define SEL4BENCH_EVENT_BR_RETURN_SPEC         0x79 /* Branch speculatively executed, procedure return */
#define SEL4BENCH_EVENT_BR_INDIRECT_SPEC       0x7a /* Branch speculatively executed, indirect branch */
#define SEL4BENCH_EVENT_ISB_SPEC               0x7c /* Barrier speculatively executed, ISB */
#define SEL4BENCH_EVENT_DSB_SPEC               0x7d /* Barrier speculatively executed, DSB */
#define SEL4BENCH_EVENT_DMB_SPEC               0x7e /* Barrier speculatively executed, DMB */
#define SEL4BENCH_EVENT_EXC_UNDEF              0x81 /* Counts the number of undefined exceptions taken locally */
#define SEL4BENCH_EVENT_EXC_SVC                0x82 /* Exception taken locally, Supervisor Call */
#define SEL4BENCH_EVENT_EXC_PABORT             0x83 /* Exception taken locally, Instruction Abort */
#define SEL4BENCH_EVENT_EXC_DABORT             0x84 /* Exception taken locally, Data Abort and SError */
#define SEL4BENCH_EVENT_EXC_IRQ                0x86 /* Exception taken locally, IRQ */
#define SEL4BENCH_EVENT_EXC_FIQ                0x87 /* Exception taken locally, FIQ */
#define SEL4BENCH_EVENT_EXC_SMC                0x88 /* Exception taken locally, Secure Monitor Call */
#define SEL4BENCH_EVENT_EXC_HVC                0x8a /* Exception taken locally, Hypervisor Call */
#define SEL4BENCH_EVENT_EXC_TRAP_PABORT        0x8b /* Exception taken, Instruction Abort not taken locally */
#define SEL4BENCH_EVENT_EXC_TRAP_DABORT        0x8c /* Exception taken, Data Abort or SError not taken locally */
#define SEL4BENCH_EVENT_EXC_TRAP_OTHER         0x8d /* Exception taken, Other traps not taken locally */
#define SEL4BENCH_EVENT_EXC_TRAP_IRQ           0x8e /* Exception taken, IRQ not taken locally */
#define SEL4BENCH_EVENT_EXC_TRAP_FIQ           0x8f /* Exception taken, FIQ not taken locally */
#define SEL4BENCH_EVENT_RC_LD_SPEC             0x90 /* Release consistency operation speculatively executed, load-acquire */
#define SEL4BENCH_EVENT_RC_ST_SPEC             0x91 /* Release consistency operation speculatively executed, store-release */
#define SEL4BENCH_EVENT_L3_CACHE_RD            0xa0 /* L3 cache read */

// Events are defined in the Morello Architecture Manual (Section 2.17).
// Events added by the Morello architecture are in the range 0x0200-0x03FF.
#define SEL4BENCH_EVENT_BR_MIS_PRED_RS        0x200 /* Branch mispredict restricted */
#define SEL4BENCH_EVENT_BR_MIS_PRED_C64       0x201 /* Branch mispredict C64 */
#define SEL4BENCH_EVENT_BR_MIS_PRED_SYS       0x202 /* Branch mispredict system permission */
#define SEL4BENCH_EVENT_PCCRF_FULL            0x203 /* PCC register file full */
#define SEL4BENCH_EVENT_EXECUTIVE_EXIT        0x205 /* Exit from Executive */
#define SEL4BENCH_EVENT_INST_SPEC_A64         0x206 /* Instructions in A64 */
#define SEL4BENCH_EVENT_INST_SPEC_C64         0x207 /* Instructions in C64 */
#define SEL4BENCH_EVENT_CID_EL0_WRITE_RETIRED 0x208 /* Instruction architecturally executed, Write to CID_EL0 */
#define SEL4BENCH_EVENT_DDC_WRITE_RETIRED     0x209 /* Instruction architecturally executed, Write to DDC_ELx, RDDC_EL0 */
#define SEL4BENCH_EVENT_DDC_READ_SPEC         0x20a /* Read from DDC_ELx, RDDC_EL0 */
#define SEL4BENCH_EVENT_INST_SPEC_CVTD        0x20b /* CVTD Instructions */
#define SEL4BENCH_EVENT_INST_SPEC_SCBNDS_NONEXACT 0x20e /* SCBNDS or SCBNDSE Instructions which do not set exact bound */
#define SEL4BENCH_EVENT_CDBM_SET_SC           0x20f /* SC set due to CDBM */
#define SEL4BENCH_EVENT_CAP_LD_SPEC           0x210 /* Capability Load Instructions */
#define SEL4BENCH_EVENT_CAP_ST_SPEC           0x211 /* Capability Store Instructions */
#define SEL4BENCH_EVENT_CAP_ALT_LD_SPEC       0x212 /* Alternate Base Capability Load Instructions */
#define SEL4BENCH_EVENT_CAP_ALT_ST_SPEC       0x213 /* Alternate Base Capability Store Instructions */
#define SEL4BENCH_EVENT_ALT_LD_SPEC           0x214 /* Alternate Base Load Instructions */
#define SEL4BENCH_EVENT_ALT_ST_SPEC           0x215 /* Alternate Base Store Instructions */
#define SEL4BENCH_EVENT_LDCT_SPEC             0x216 /* LDCT Instructions */
#define SEL4BENCH_EVENT_LDCT_NO_CAP_SPEC      0x217 /* LDCT Instructions When Capability Tags are Zero */
#define SEL4BENCH_EVENT_DC_ZVA_RET            0x218 /* Data Cache Zero */
#define SEL4BENCH_EVENT_LDCT_REFILL           0x21a /* Data cache refill due to LDCT */
#define SEL4BENCH_EVENT_STCT_REFILL           0x21b /* Data cache refill due to STCT */
#define SEL4BENCH_EVENT_L1D_CACHE_RD_CTAG     0x21c /* Attributable Level 1 data cache access, read, valid capability */
#define SEL4BENCH_EVENT_L1D_CACHE_WR_CTAG     0x21d /* Attributable Level 1 data cache access, write, valid capability */
#define SEL4BENCH_EVENT_L1D_CACHE_WB_CTAG     0x21e /* Attributable Level 1 data cache write-back, valid capability */
#define SEL4BENCH_EVENT_L1D_CACHE_REFILL_RD_CTAG    0x21f /* Attributable Level 1 data cache refill, capability */
#define SEL4BENCH_EVENT_L1D_CACHE_REFILL_WR_CTAG    0x220 /* Attributable Level 1 data cache refill, capability */
#define SEL4BENCH_EVENT_L1D_CACHE_REFILL_INNER_CTAG 0x221 /* Attributable Level 1 data cache refill, inner, valid capability */
#define SEL4BENCH_EVENT_L1D_CACHE_REFILL_OUTER_CTAG 0x222 /* Attributable Level 1 data cache refill, outer, valid capability */
#define SEL4BENCH_EVENT_L1D_CACHE_WB_VICTIM_CTAG    0x223 /* Attributable Level 1 data cache Write-Back, victim, valid capability */
#define SEL4BENCH_EVENT_L1D_CACHE_WB_CLEAN_CTAG     0x224 /* Attributable Level 1 data cache Write-Back, cleaning, and coherency, valid capability */
#define SEL4BENCH_EVENT_L2D_CACHE_RD_CTAG           0x226 /* ttributable Level 2 data cache access, read, valid capability */
#define SEL4BENCH_EVENT_L2D_CACHE_WR_CTAG           0x227 /* Attributable Level 2 data cache access, write, valid capability */
#define SEL4BENCH_EVENT_L2D_CACHE_REFILL_RD_CTAG    0x228 /* Attributable Level 2 data cache refill, valid capability */
#define SEL4BENCH_EVENT_L2D_CACHE_WB_VICTIM_CTAG    0x22a /* Attributable Level 2 data cache Write-Back, victim, valid capability */
#define SEL4BENCH_EVENT_L2D_CACHE_WB_CLEAN_CTAG     0x22b /* Attributable Level 2 data cache Write-Back, cleaning and coherency, valid capability */
#define SEL4BENCH_EVENT_L2D_CACHE_INVAL_CTAG        0x22c /* Attributable Level 2 data cache invalidate, valid capability */
#define SEL4BENCH_EVENT_BUS_ACCESS_RD_CTAG    0x22d /* Bus access, read, valid capability */
#define SEL4BENCH_EVENT_BUS_ACCESS_WR_CTAG    0x22e /* Bus access, write, valid capability */
#define SEL4BENCH_EVENT_CNT_ST_ZERO_BYTE      0x22f /* Store of zeros */
#define SEL4BENCH_EVENT_CNT_ST_ZERO_16_BYTES  0x230 /* Store of zeros, 16 byte stores */
#define SEL4BENCH_EVENT_MEM_ACCESS_RD_CTAG    0x233 /* Data memory access, read, valid capability */
#define SEL4BENCH_EVENT_MEM_ACCESS_WR_CTAG    0x234 /* Data memory access, write, valid capability */
#define SEL4BENCH_EVENT_CAP_MEM_ACCESS_RD     0x235 /* Data memory access, read, capability */
#define SEL4BENCH_EVENT_CAP_MEM_ACCESS_WR     0x236 /* Data memory access, write, capability */
#define SEL4BENCH_EVENT_INST_SPEC_RESTRICTED  0x237 /* Instructions in Restricted */
#define SEL4BENCH_EVENT_LD_CAP_PERM_CLR_CTAG  0x238 /* Load permission cleared */

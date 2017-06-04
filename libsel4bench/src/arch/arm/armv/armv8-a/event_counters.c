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

#include <utils/util.h>

#include "../../event_counters.h"

const char* const sel4bench_arch_event_counter_data[] = {
    NAME_EVENT(SOFTWARE_INCREMENT        , "Instruction architecturally executed, condition code check pass, software increment"),
    NAME_EVENT(CACHE_L1I_MISS            , "Level 1 instruction cache refill"),
    NAME_EVENT(TLB_L1I_MISS              , "Level 1 instruction TLB refill"),
    NAME_EVENT(CACHE_L1D_MISS            , "Level 1 data cache refill"),
    NAME_EVENT(CACHE_L1D_HIT             , "Level 1 data cache access"),
    NAME_EVENT(TLB_L1D_MISS              , "Level 1 data TLB refill"),
    NAME_EVENT(MEMORY_READ               , "Instruction architecturally executed, condition code check pass, load"),
    NAME_EVENT(MEMORY_WRITE              , "Instruction architecturally executed, condition code check pass, store"),
    NAME_EVENT(EXECUTE_INSTRUCTION       , "Instruction architecturally executed"),
    NAME_EVENT(EXCEPTION                 , "Exception taken"),
    NAME_EVENT(EXCEPTION_RETURN          , "Instruction architecturally executed, condition code check pass, exception return"),
    NAME_EVENT(CONTEXTIDR_WRITE          , "Instruction architecturally executed, condition code check pass, write to CONTEXTIDR"),
    NAME_EVENT(SOFTWARE_PC_CHANGE        , "Instruction architecturally executed, condition code check pass, software change of the PC"),
    NAME_EVENT(EXECUTE_BRANCH_IMM        , "Instruction architecturally executed, immediate branch"),
    NAME_EVENT(FUNCTION_RETURN           , "Instruction architecturally executed, condition code check pass, procedure return"),
    NAME_EVENT(MEMORY_ACCESS_UNALIGNED   , "Instruction architecturally executed, condition code check pass, unaligned load or store"),
    NAME_EVENT(BRANCH_MISPREDICT         , "Mispredicted or not predicted branch speculatively executed"),
    NAME_EVENT(CCNT                      , "Cycle"),
    NAME_EVENT(EXECUTE_BRANCH_PREDICTABLE, "Predictable branch speculatively executed"),
    NAME_EVENT(MEMORY_ACCESS                , "Data memory access"),
    NAME_EVENT(L1I_CACHE                 , "Level 1 instruction cache access"),
    NAME_EVENT(L1D_CACHE_WB              , "Level 1 data cache write-back"),
    NAME_EVENT(L2D_CACHE                 , "Level 2 data cache access"),
    NAME_EVENT(L2D_CACHE_REFILL          , "Level 2 data cache refill"),
    NAME_EVENT(L2D_CACHE_WB              , "Level 2 data cache write-back"),
    NAME_EVENT(BUS_ACCESS                , "Bus access"),
    NAME_EVENT(MEMORY_ERROR              , "Local memory error"),
    NAME_EVENT(INST_SPEC                 , "Instruction speculatively executed"),
    NAME_EVENT(TTBR_WRITE_RETIRED        , "Instruction architecturally executed, condition code check pass, write to TTBR"),
    NAME_EVENT(BUS_CYCLES                , "Bus cycle"),
    NAME_EVENT(CHAIN                     , "For odd-numbered counters, increments the count by one for each overflow of the preceding even-numbered counter. For even-numbered counters there is no increment."),
    NAME_EVENT(L1D_CACHE_ALLOCATE        , "Level 1 data cache allocation without refill"),
    NAME_EVENT(L2D_CACHE_ALLOCATE        , "Level 2 data cache allocation without refill"),
    NAME_EVENT(BR_RETIRED                , "Instruction architecturally executed, branch"),
    NAME_EVENT(BR_MIS_PRED_RETIRED       , "Instruction architecturally executed, mispredicted branch"),
    NAME_EVENT(STALL_FRONTEND            , "No operation issued due to the frontend stall"),
    NAME_EVENT(STALL_BACKEND             , "No operation issued due to backend stall"),
    NAME_EVENT(L1D_TLB                   , "Level 1 data or unified TLB access"),
    NAME_EVENT(L1I_TLB                   , "Level 1 instruction TLB access"),
    NAME_EVENT(L2I_CACHE                 , "Attributable Level 2 instruction cache access"),
    NAME_EVENT(L2I_CACHE_REFILL          , "Attributable Level 2 instruction cache refill"),
    NAME_EVENT(L3D_CACHE_ALLOCATE        , "Attributable Level 3 data or unified cache allocation without refill"),
    NAME_EVENT(L3D_CACHE_REFILL          , "Attributable Level 3 data or unified cache refill"),
    NAME_EVENT(L3D_CACHE                 , "Attributable Level 3 data or unified cache access"),
    NAME_EVENT(L3D_CACHE_WB              , "Attributable Level 3 data or unified cache write-back"),
    NAME_EVENT(L2D_TLB_REFILL            , "Attributable Level 2 data or unified TLB refill"),
    NAME_EVENT(L2I_TLB_REFILL            , "Attributable Level 2 instruction TLB refill"),
    NAME_EVENT(L2D_TLB                   , "Attributable Level 2 data or unified TLB access"),
    NAME_EVENT(L2I_TLB                   , "Attributable Level 2 instruction TLB access")
};

int
sel4bench_arch_get_num_counters(void)
{
    return ARRAY_SIZE(sel4bench_arch_event_counter_data);
}

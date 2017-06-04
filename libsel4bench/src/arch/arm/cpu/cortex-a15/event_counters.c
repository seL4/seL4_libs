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

#include "../../event_counters.h"

const char* const sel4bench_cpu_event_counter_data[] = {
    NAME_EVENT(L1D_CACHE_LD         , "Level 1 data cache access, read"),
    NAME_EVENT(L1D_CACHE_ST         , "Level 1 data cache access, write"),
    NAME_EVENT(L1D_CACHE_REFILL_LD  , "Level 1 data cache refill, read"),
    NAME_EVENT(L1D_CACHE_REFILL_ST  , "Level 1 data cache refill, write"),
    NAME_EVENT(L1D_CACHE_WB_VICTIM  , "Level 1 data cache write-back, victim"),
    NAME_EVENT(L1D_CACHE_WB_CLEAN   , "Level 1 data cache write-back, cleaning and coherency"),
    NAME_EVENT(L1D_CACHE_INVAL      , "Level 1 data cache invalidate"),
    NAME_EVENT(L1D_TLB_REFILL_LD    , "Level 1 data TLB refill, read"),
    NAME_EVENT(L1D_TLB_REFILL_ST    , "Level 1 data TLB refill, write"),
    NAME_EVENT(L2D_CACHE_LD         , "Level 2 data cache access, read"),
    NAME_EVENT(L2D_CACHE_ST         , "Level 2 data cache access, write"),
    NAME_EVENT(L2D_CACHE_REFILL_LD  , "Level 2 data cache refill, read"),
    NAME_EVENT(L2D_CACHE_REFILL_ST  , "Level 2 data cache refill, write"),
    NAME_EVENT(L2D_CACHE_WB_VICTIM  , "Level 2 data cache write-back, victim"),
    NAME_EVENT(L2D_CACHE_WB_CLEAN   , "Level 2 data cache write-back, cleaning and coherency"),
    NAME_EVENT(L2D_CACHE_INVAL      , "Level 2 data cache invalidate"),
    NAME_EVENT(BUS_ACCESS_LD        , "Bus access, read"),
    NAME_EVENT(BUS_ACCESS_ST        , "Bus access, write"),
    NAME_EVENT(BUS_ACCESS_SHARED    , "Bus access, Normal, Cacheable, Shareable"),
    NAME_EVENT(BUS_ACCESS_NOT_SHARED, "Bus access, not Normal, Cacheable, Shareable"),
    NAME_EVENT(BUS_ACCESS_NORMAL    , "Bus access, normal"),
    NAME_EVENT(BUS_ACCESS_PERIPH    , "Bus access, peripheral"),
    NAME_EVENT(MEM_ACCESS_LD        , "Data memory access, read"),
    NAME_EVENT(MEM_ACCESS_ST        , "Data memory access, write"),
    NAME_EVENT(UNALIGNED_LD_SPEC    , "Unaligned access, read"),
    NAME_EVENT(UNALIGNED_ST_SPEC    , "Unaligned access, write"),
    NAME_EVENT(UNALIGNED_LDST_SPEC  , "Unaligned access"),
    NAME_EVENT(LDREX_SPEC           , "Exclusive instruction speculatively executed, LDREX"),
    NAME_EVENT(STREX_PASS_SPEC      , "Exclusive instruction speculatively executed, STREX pass"),
    NAME_EVENT(STREX_FAIL_SPEC      , "Exclusive instruction speculatively executed, STREX fail"),
    NAME_EVENT(LD_SPEC              , "Instruction speculatively executed, load"),
    NAME_EVENT(ST_SPEC              , "Instruction speculatively executed, store"),
    NAME_EVENT(LDST_SPEC            , "Instruction speculatively executed, load or store"),
    NAME_EVENT(DP_SPEC              , "Instruction speculatively executed, integer data processing"),
    NAME_EVENT(ASE_SPEC             , "Instruction speculatively executed, Advanced SIMD Extension"),
    NAME_EVENT(VFP_SPEC             , "Instruction speculatively executed, Floating-point Extension"),
    NAME_EVENT(PC_WRITE_SPEC        , "Instruction speculatively executed, software change of the PC"),
    NAME_EVENT(BR_IMMED_SPEC        , "Branch speculatively executed, immediate branch"),
    NAME_EVENT(BR_RETURN_SPEC       , "Branch speculatively executed, procedure return"),
    NAME_EVENT(BR_INDIRECT_SPEC     , "Branch speculatively executed, indirect branch"),
    NAME_EVENT(ISB_SPEC             , "Barrier speculatively executed, ISB"),
    NAME_EVENT(DSB_SPEC             , "Barrier speculatively executed, DSB"),
    NAME_EVENT(DMB_SPEC             , "Barrier speculatively executed, DMB")
};

int
sel4bench_cpu_get_num_counters(void)
{
    return ARRAY_SIZE(sel4bench_cpu_event_counter_data);
}

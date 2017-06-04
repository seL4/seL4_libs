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
    NAME_EVENT(CACHE_L1I_MISS        , "CACHE_L1I_MISS"),
    NAME_EVENT(STALL_INSTRUCTION     , "STALL_INSTRUCTION"),
    NAME_EVENT(STALL_DATA            , "STALL_DATA"),
    NAME_EVENT(TLB_L1I_MISS          , "TLB_L1I_MISS"),
    NAME_EVENT(TLB_L1D_MISS          , "TLB_L1D_MISS"),
    NAME_EVENT(EXECUTE_BRANCH        , "EXECUTE_BRANCH"),
    NAME_EVENT(BRANCH_MISPREDICT     , "BRANCH_MISPREDICT"),
    NAME_EVENT(EXECUTE_INSTRUCTION   , "EXECUTE_INSTRUCTION"),
    NAME_EVENT(CACHE_L1D_HIT         , "CACHE_L1D_HIT"),
    NAME_EVENT(CACHE_L1D_ACCESS      , "CACHE_L1D_ACCESS"),
    NAME_EVENT(CACHE_L1D_MISS        , "CACHE_L1D_MISS"),
    NAME_EVENT(CACHE_L1D_WRITEBACK_HL, "CACHE_L1D_WRITEBACK_HL"),
    NAME_EVENT(SOFTWARE_PC_CHANGE    , "SOFTWARE_PC_CHANGE"),
    NAME_EVENT(TLB_L2_MISS           , "TLB_L2_MISS"),
    NAME_EVENT(MEMORY_ACCESS         , "MEMORY_ACCESS"),
    NAME_EVENT(STALL_LSU_BUSY        , "STALL_LSU_BUSY"),
    NAME_EVENT(WRITE_BUFFER_DRAIN    , "WRITE_BUFFER_DRAIN"),
    NAME_EVENT(ETMEXTOUT_0           , "ETMEXTOUT_0"),
    NAME_EVENT(ETMEXTOUT_1           , "ETMEXTOUT_1"),
    NAME_EVENT(ETMEXTOUT             , "ETMEXTOUT"),
    NAME_EVENT(CCNT                  , "CCNT")
};

const char* const sel4bench_cpu_event_counter_data[] = {
};

int
sel4bench_arch_get_num_counters(void)
{
    return ARRAY_SIZE(sel4bench_arch_event_counter_data);
}

int
sel4bench_cpu_get_num_counters(void)
{
    return ARRAY_SIZE(sel4bench_cpu_event_counter_data);
}

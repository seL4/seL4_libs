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
    NAME_EVENT(JAVA_BYTECODE_EXECUTE   , "Java bytecode executed"),
    NAME_EVENT(SW_JAVA_BYTECODE_EXECUTE, "Software Java bytecode executed"),
    NAME_EVENT(JAZELLE_BW_BRANCHES     , "Jazelle backward branches executed"),
    NAME_EVENT(LINEFILL_MISS           , "Coherent linefill miss"),
    NAME_EVENT(LINEFILL_HIT            , "Coherent linefill hit"),
    NAME_EVENT(ICACHE_STALL            , "Instruction cache dependent stall cycles"),
    NAME_EVENT(DCACHE_STALL            , "Data cache dependent stall cycles"),
    NAME_EVENT(TLBMISS_STALL           , "Main TLB miss stall cycles"),
    NAME_EVENT(STREX_PASSED            , "STREX passed"),
    NAME_EVENT(STREX_FAILED            , "STREX failed"),
    NAME_EVENT(DATA_EVICT              , "Data eviction"),
    NAME_EVENT(ISSUE_NO_DISPATCH       , "Issue does not dispatch any instruction"),
    NAME_EVENT(ISSUE_EMPTY             , "Issue is empty"),
    NAME_EVENT(RENAME_INST             , "Instructions coming out of the core renaming stage"),
    NAME_EVENT(PREDICTABLE_FUNCTION_RET, "Predictable function returns"),
    NAME_EVENT(MAIN_EXEC_INST          , "Main execution unit instructions"),
    NAME_EVENT(SECOND_EXEC_INST        , "Second execution unit instructions"),
    NAME_EVENT(LOADSTORE_INST          , "Load/Store Instructions"),
    NAME_EVENT(FLOAT_INST              , "Floating-point instructions"),
    NAME_EVENT(NEON_INST               , "NEON instructions"),
    NAME_EVENT(PLD_STALL               , "Processor stall - PLDs"),
    NAME_EVENT(WRITE_STALL             , "Processor stall - a write to memory"),
    NAME_EVENT(ITLBMISS_STALL          , "Processor stall - instruction side main TLB miss"),
    NAME_EVENT(DTLBMISS_STALL          , "Processor stall - data side main TLB miss"),
    NAME_EVENT(IUTLBMISS_STALL         , "Processor stall - instruction micro TLB miss"),
    NAME_EVENT(DUTLBMISS_STALL         , "Processor stall - data micro TLB miss"),
    NAME_EVENT(DMB_STALL               , "Processor stall - DMB"),
    NAME_EVENT(INTEGER_CLOCK_ENABLED   , "Integer clock enabled"),
    NAME_EVENT(DATA_CLOCK_ENABLED      , "Data Engine clock enabled"),
    NAME_EVENT(ISB_INST                , "ISB instructions"),
    NAME_EVENT(DSB_INST                , "DSB instructions"),
    NAME_EVENT(DMB_INST                , "DMB instructions"),
    NAME_EVENT(EXT_IRQ                 , "External interrupts"),
    NAME_EVENT(PLE_CACHELINE_COMPLETED , "PLE cache line request completed"),
    NAME_EVENT(PLE_CACHELINE_SKIPPED   , "PLE cache line request skipped"),
    NAME_EVENT(PLE_FIFO_FLUSH          , "PLE FIFO flush"),
    NAME_EVENT(PLE_COMPLETED           , "PLE request completed"),
    NAME_EVENT(PLE_FIFO_OVERFLOW       , "PLE FIFO overflow"),
    NAME_EVENT(PLE_REQUESTS            , "PLE request programmed")
};

int
sel4bench_cpu_get_num_counters(void)
{
    return ARRAY_SIZE(sel4bench_cpu_event_counter_data);
}

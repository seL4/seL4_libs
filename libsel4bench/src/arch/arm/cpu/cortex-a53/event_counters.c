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
    NAME_EVENT(BUS_ACCESS_LD        , "Bus access, read"),
    NAME_EVENT(BUS_ACCESS_ST        , "Bus access, write"),
    NAME_EVENT(BR_INDIRECT_SPEC     , "Branch speculatively executed, indirect branch"),
    NAME_EVENT(EXC_IRQ              , "Exception taken, IRQ"),
    NAME_EVENT(EXC_FIQ              , "Exception taken, FIQ")
};

int
sel4bench_cpu_get_num_counters(void)
{
    return ARRAY_SIZE(sel4bench_cpu_event_counter_data);
}

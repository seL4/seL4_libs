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
#include "event_counters.h"

const char*
sel4bench_arch_get_counter_description(counter_t counter)
{
    if (counter < 0) {
        return NULL;
    } else if (counter < sel4bench_arch_get_num_counters()) {
        return sel4bench_arch_event_counter_data[counter];
    } else if (counter < sel4bench_cpu_get_num_counters()) {
        return sel4bench_cpu_event_counter_data[counter];
    } else {
        return NULL;
    }
}

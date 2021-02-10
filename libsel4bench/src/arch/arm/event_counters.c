/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
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

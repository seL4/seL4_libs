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

#include <sel4bench/sel4bench.h>
#include "../../event_counters.h"

#define EVENT_COUNTER_FORMAT(id, name) [id] = name

extern const char* const sel4bench_arch_event_counter_data[];
extern const char* const sel4bench_cpu_event_counter_data[];

extern int sel4bench_arch_get_num_counters(void);
extern int sel4bench_cpu_get_num_counters(void);

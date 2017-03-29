/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <sel4bench/sel4bench.h>
#include "../event_counters.h"

#define EVENT_COUNTER_FORMAT(id, name) [id] = name

extern const char* const sel4bench_arch_event_counter_data[];
extern const char* const sel4bench_cpu_event_counter_data[];

extern int sel4bench_arch_get_num_counters(void);
extern int sel4bench_cpu_get_num_counters(void);


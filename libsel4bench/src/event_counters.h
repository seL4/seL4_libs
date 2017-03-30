/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */


#include <sel4/types.h>

#define NAME_EVENT(id, name) EVENT_COUNTER_FORMAT(SEL4BENCH_EVENT_##id, name)

const char* sel4bench_arch_get_counter_description(counter_t counter);


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

#include <sel4/types.h>

#define NAME_EVENT(id, name) EVENT_COUNTER_FORMAT(SEL4BENCH_EVENT_##id, name)

const char* sel4bench_arch_get_counter_description(counter_t counter);

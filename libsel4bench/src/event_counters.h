/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <sel4/types.h>

#define NAME_EVENT(id, name) EVENT_COUNTER_FORMAT(SEL4BENCH_EVENT_##id, name)

const char* sel4bench_arch_get_counter_description(counter_t counter);

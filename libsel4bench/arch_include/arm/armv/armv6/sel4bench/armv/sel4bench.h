/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <sel4bench/cpu/sel4bench.h>

#define SEL4BENCH_READ_CCNT(var) do { \
    asm volatile("b 2f\n\
                1:mrc p15, 0, %[counter], c15, c12," SEL4BENCH_ARM1136_COUNTER_CCNT "\n\
                  bx lr\n\
                2:sub r8, pc, #16\n\
                  .word 0xe7f000f0" \
        : [counter] "=r"(var) \
        : \
        : "r8", "lr"); \
} while(0)

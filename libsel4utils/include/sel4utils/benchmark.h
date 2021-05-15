/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once
#include <autoconf.h>
#include <sel4utils/gen_config.h>
#ifdef CONFIG_BENCHMARK_TRACEPOINTS
#include <inttypes.h>
#include <stdio.h>

#include <sel4/simple_types.h>
#include <sel4/benchmark_tracepoints_types.h>

/**
 * Dump the benchmark log. The kernel must be compiled with CONFIG_MAX_NUM_TRACE_POINTS > 0,
 * either edit .config manually or add it using "make menuconfig" by selecting
 * "Kernel" -> "seL4 System Parameters" -> "Adds a log buffer to the kernel for instrumentation."
 */
static inline void seL4_BenchmarkTraceDumpFullLog(benchmark_tracepoint_log_entry_t *logBuffer, size_t logSize)
{
    seL4_Word index = 0;
    FILE *fd = stdout;

    while ((index * sizeof(benchmark_tracepoint_log_entry_t)) < logSize) {
        if (logBuffer[index].duration != 0) {
            fprintf(fd, "tracepoint id = %u \tduration = %u\n", logBuffer[index].id, logBuffer[index].duration);
        }
        index++;
    }

    fprintf(fd, "Dumped entire log, size %" PRIu32 "\n", index);
}
#endif /* CONFIG_BENCHMARK_TRACEPOINTS */

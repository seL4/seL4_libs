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
#include <sel4/arch/constants.h>
#include <sel4/simple_types.h>
#include <sel4/benchmark_tracepoints_types.h>
#include <sel4/arch/syscalls.h>
#include <inttypes.h>

#if CONFIG_MAX_NUM_TRACE_POINTS > 0
#define KERNEL_MAX_NUM_LOG_ENTRIES (BIT(seL4_LargePageBits) / sizeof(benchmark_tracepoint_log_entry_t))
typedef benchmark_tracepoint_log_entry_t  kernel_log_entry_t;
#else
#define KERNEL_MAX_NUM_LOG_ENTRIES 0
typedef void *kernel_log_entry_t;
#endif

/* Copies up to n entries from the kernel's internal log to the specified array,
 * returning the number of entries copied.
 */
unsigned int kernel_logging_sync_log(kernel_log_entry_t log[], unsigned int n);

/* Returns the key field of a log entry. */
static inline seL4_Word
kernel_logging_entry_get_key(kernel_log_entry_t *entry)
{
#if CONFIG_MAX_NUM_TRACE_POINTS > 0
    return entry->id;
#else
    return 0;
#endif
}

/* Sets the key field of a log entry to a given value. */
static inline void
kernel_logging_entry_set_key(kernel_log_entry_t *entry, seL4_Word key)
{
#if CONFIG_MAX_NUM_TRACE_POINTS > 0
    entry->id = key;
#endif
}

/* Returns the data field of a log entry. */
static inline seL4_Word
kernel_logging_entry_get_data(kernel_log_entry_t *entry)
{
#if CONFIG_MAX_NUM_TRACE_POINTS > 0
    return entry->duration;
#else
    return 0;
#endif
}

/* Sets the data field of a log entry to a given value. */
static inline void
kernel_logging_entry_set_data(kernel_log_entry_t *entry, seL4_Word data)
{
#if CONFIG_MAX_NUM_TRACE_POINTS > 0
    entry->duration = data;
#endif
}

/* Resets the log buffer to contain no entries. */
static inline void
kernel_logging_reset_log(void)
{
#ifdef CONFIG_ENABLE_BENCHMARKS
    seL4_BenchmarkResetLog();
#endif /* CONFIG_ENABLE_BENCHMARKS */
}

/* Calls to kernel_logging_sync_log will extract entries created before
 * the most-recent call to this function. Call this function before calling
 * kernel_logging_sync_log. */
static inline void
kernel_logging_finalize_log(void)
{
#ifdef CONFIG_ENABLE_BENCHMARKS
    seL4_BenchmarkFinalizeLog();
#endif /* CONFIG_ENABLE_BENCHMARKS */
}

/* Tell the kernel about the allocated user-level buffer
 * so that it can write to it. Note, this function has to
 * be called before kernel_logging_reset_log.
 *
 * @logBuffer_cap should be a cap of a large frame size.
 */
static inline seL4_Error
kernel_logging_set_log_buffer(seL4_CPtr logBuffer_cap)
{
#ifdef CONFIG_BENCHMARK_USE_KERNEL_LOG_BUFFER
    return seL4_BenchmarkSetLogBuffer(logBuffer_cap);
#else
    return seL4_NoError;
#endif /* CONFIG_BENCHMARK_USE_KERNEL_LOG_BUFFER */
}

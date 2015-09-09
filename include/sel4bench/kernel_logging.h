/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __KERNEL_LOGGING_H__
#define __KERNEL_LOGGING_H__

/* Utilities for extracting logs from the kernel */

#include <sel4/types.h>
#include <sel4/arch/constants.h>
#include <sel4/simple_types.h>
#include <sel4/arch/syscalls.h>

#if CONFIG_MAX_NUM_TRACE_POINTS > 0
#define KERNEL_MAX_NUM_LOG_ENTRIES (seL4_LogBufferSize / sizeof(seL4_LogEntry))
typedef seL4_LogEntry kernel_log_entry_t;
#else
#define KERNEL_MAX_NUM_LOG_ENTRIES 0
typedef void *kernel_log_entry_t;
#endif

/* Copies up to n entries from the kernel's internal log to the specified array,
 * returning the number of entries copied.
 */
unsigned int kernel_logging_sync_log(kernel_log_entry_t log[], unsigned int n);

static inline seL4_Word
kernel_logging_entry_get_key(kernel_log_entry_t *entry)
{
#if CONFIG_MAX_NUM_TRACE_POINTS > 0
    return entry->key;
#else
    return 0;
#endif
}

static inline void
kernel_logging_entry_set_key(kernel_log_entry_t *entry, seL4_Word key)
{
#if CONFIG_MAX_NUM_TRACE_POINTS > 0
    entry->key = key;
#endif
}

static inline seL4_Word
kernel_logging_entry_get_data(kernel_log_entry_t *entry)
{
#if CONFIG_MAX_NUM_TRACE_POINTS > 0
    return entry->data;
#else
    return 0;
#endif
}

static inline void
kernel_logging_entry_set_data(kernel_log_entry_t *entry, seL4_Word data)
{
#if CONFIG_MAX_NUM_TRACE_POINTS > 0
    entry->data = data;
#endif
}

static inline void
kernel_logging_reset_log(void)
{
#if CONFIG_MAX_NUM_TRACE_POINTS > 0
    seL4_BenchmarkResetLog();
#endif
}

static inline void
kernel_logging_finalize_log(void)
{
#if CONFIG_MAX_NUM_TRACE_POINTS > 0
    seL4_BenchmarkFinalizeLog();
#endif
}

static inline unsigned int
kernel_logging_log_size(void)
{
#if CONFIG_MAX_NUM_TRACE_POINTS > 0
    return seL4_BenchmarkLogSize();
#else
    return 0;
#endif
}

static inline unsigned int
kernel_logging_dump_log(unsigned int start, unsigned int size)
{
#if CONFIG_MAX_NUM_TRACE_POINTS > 0
    return seL4_BenchmarkDumpLog(start, size);
#else
    return 0;
#endif
}

#endif

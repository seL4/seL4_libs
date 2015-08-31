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
#include <sel4/constants.h>
#include <sel4/simple_types.h>
#include <sel4/arch/syscalls.h>
#include <sel4bench/logging.h>

/* Must be at least the number of logs that can be stored by the kernel */
#define KERNEL_MAX_LOG_SIZE (seL4_LogBufferSize / sizeof(seL4_LogEntry))

/* Copies up to n entries from the kernel's internal log to the specified array,
 * returning the number of entries copied.
 */
unsigned int kernel_logging_sync_log(seL4_LogEntry log[], unsigned int n);

static inline void kernel_logging_reset_log(void) {
#ifdef CONFIG_BENCHMARK
    seL4_BenchmarkResetLog();
#endif
}

static inline void kernel_logging_finalize_log(void) {
#ifdef CONFIG_BENCHMARK
    seL4_BenchmarkFinalizeLog();
#endif
}

static inline unsigned int kernel_logging_log_size(void) {
#ifdef CONFIG_BENCHMARK
    return seL4_BenchmarkLogSize();
#else
    return 0;
#endif
}

static inline unsigned int kernel_logging_dump_log(unsigned int start, unsigned int size) {
#ifdef CONFIG_BENCHMARK
    return seL4_BenchmarkDumpLog(start, size);
#else
    return 0;
#endif
}

#endif

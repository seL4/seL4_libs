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

#include <sel4/simple_types.h>
#include <sel4/arch/syscalls.h>
#include <sel4bench/logging.h>

/* Must be at least the number of logs that can be stored by the kernel */
#define KERNEL_LOG_BUFFER_SIZE ((1<<20) / sizeof(log_entry_t))

/* Copies up to n entries from the kernel's internal log to the specified array,
 * returning the number of entries copied.
 */
unsigned int kernel_logging_sync_log(log_entry_t log[], unsigned int n);

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

#endif

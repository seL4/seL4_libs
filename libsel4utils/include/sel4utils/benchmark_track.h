/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once
#include <autoconf.h>
#include <sel4utils/gen_config.h>
#ifdef CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES

#include <stdio.h>
#include <sel4/types.h>
#include <sel4/benchmark_track_types.h>

/* Print out a summary of what has been tracked */
static inline void seL4_BenchmarkTrackDumpSummary(benchmark_track_kernel_entry_t *logBuffer, size_t logSize)
{
    seL4_Word index = 0;
    seL4_Word syscall_entries = 0;
    seL4_Word interrupt_entries = 0;
    seL4_Word userlevelfault_entries = 0;
    seL4_Word vmfault_entries = 0;

    /* Default driver to use for output now is serial.
     * Change this to use other drivers than serial, i.e ethernet
     */
    FILE *fd = stdout;

    while (logBuffer[index].start_time != 0 && (index * sizeof(benchmark_track_kernel_entry_t)) < logSize) {
        if (logBuffer[index].entry.path == Entry_Syscall) {
            syscall_entries++;
        } else if (logBuffer[index].entry.path == Entry_Interrupt) {
            interrupt_entries++;
        } else if (logBuffer[index].entry.path == Entry_UserLevelFault) {
            userlevelfault_entries++;
        } else if (logBuffer[index].entry.path == Entry_VMFault) {
            vmfault_entries++;
        }
        index++;
    }

    fprintf(fd, "Number of system call invocations %lu\n", (unsigned long) syscall_entries);
    fprintf(fd, "Number of interrupt invocations %lu\n", (unsigned long) interrupt_entries);
    fprintf(fd, "Number of user-level faults %lu\n", (unsigned long) userlevelfault_entries);
    fprintf(fd, "Number of VM faults %lu\n", (unsigned long) vmfault_entries);
}

/* Print out logged system call invocations */
static inline void seL4_BenchmarkTrackDumpFullSyscallLog(benchmark_track_kernel_entry_t *logBuffer, size_t logSize)
{
    seL4_Word index = 0;
    FILE *fd = stdout;

    /* Get details of each system call invocation */
    fprintf(fd,
            "-----------------------------------------------------------------------------------------------------------------------------\n");
    fprintf(fd, "|     %-15s|     %-15s|     %-15s|     %-15s|     %-15s|     %-15s|     %-15s|\n",
            "Log ID", "System Call ID", "Start Time", "Duration", "Capability Type",
            "Invocation Tag",  "Fastpath?");
    fprintf(fd,
            "-----------------------------------------------------------------------------------------------------------------------------\n");

    while (logBuffer[index].start_time != 0 && (index * sizeof(benchmark_track_kernel_entry_t)) < logSize) {
        if (logBuffer[index].entry.path == Entry_Syscall) {
            fprintf(fd, "|     %-15lu|     %-15lu|     %-15"PRIu64"|     %-15lu|     %-15lu|     %-15lu|     %-15lu|\n",
                    (unsigned long) index,
                    (unsigned long) logBuffer[index].entry.syscall_no,
                    logBuffer[index].start_time,
                    (unsigned long) logBuffer[index].duration,
                    (unsigned long) logBuffer[index].entry.cap_type,
                    (unsigned long) logBuffer[index].entry.invocation_tag,
                    (unsigned long) logBuffer[index].entry.is_fastpath);
        }
        index++;
    }
}

/* Print out logged interrupt invocations */
static inline void seL4_BenchmarkTrackDumpFullInterruptLog(benchmark_track_kernel_entry_t *logBuffer, size_t logSize)
{
    seL4_Word index = 0;
    FILE *fd = stdout;

    /* Get details of each invocation */
    fprintf(fd,
            "-----------------------------------------------------------------------------------------------------------------------------\n");
    fprintf(fd, "|     %-15s|     %-15s|     %-15s|     %-15s|\n", "Log ID", "IRQ", "Start Time",
            "Duration");
    fprintf(fd,
            "-----------------------------------------------------------------------------------------------------------------------------\n");

    while (logBuffer[index].start_time != 0 && (index * sizeof(benchmark_track_kernel_entry_t)) < logSize) {
        if (logBuffer[index].entry.path == Entry_Interrupt) {
            fprintf(fd, "|     %-15lu|     %-15lu|     %-15"PRIu64"|     %-15lu|\n", \
                    (unsigned long) index,
                    (unsigned long) logBuffer[index].entry.word,
                    logBuffer[index].start_time,
                    (unsigned long) logBuffer[index].duration);
        }
        index++;
    }
}
#endif /* CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES */

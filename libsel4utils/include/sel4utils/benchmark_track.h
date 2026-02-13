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

    fprintf(fd, "Number of system call invocations: %"SEL4_PRIu_word"\n", syscall_entries);
    fprintf(fd, "Number of interrupt invocations: %"SEL4_PRIu_word"\n", interrupt_entries);
    fprintf(fd, "Number of user-level faults: %"SEL4_PRIu_word"\n", userlevelfault_entries);
    fprintf(fd, "Number of VM faults: %"SEL4_PRIu_word"\n", vmfault_entries);
}

/* Print out logged system call invocations */
static inline void seL4_BenchmarkTrackDumpFullSyscallLog(benchmark_track_kernel_entry_t *logBuffer, size_t logSize)
{
    seL4_Word index = 0;
    FILE *fd = stdout;

    /* Get details of each system call invocation */
    fprintf(fd,
            "-----------------------------------------------------------------------------------------------------------------------------\n");
    fprintf(fd, " %-10s | %-7s | %-20s | %-10s | %-3s | %-7s| %-8s\n",
            "Log ID", "SysCall", "Start Time", "Duration", "Cap",
            "Tag",  "Fastpath?");
    fprintf(fd,
            "-----------------------------------------------------------------------------------------------------------------------------\n");

    while (logBuffer[index].start_time != 0 && (index * sizeof(benchmark_track_kernel_entry_t)) < logSize) {
        if (logBuffer[index].entry.path == Entry_Syscall) {
            fprintf(fd, " %-10"SEL4_PRIu_word" | %-7u | %-20"PRIu64" | %-10u | %-3u | %-7u | %-9s\n",
                    index,
                    (unsigned int)logBuffer[index].entry.syscall_no, /* 4 bit */
                    logBuffer[index].start_time, /* 64 bit */
                    logBuffer[index].duration, /* 32 bit */
                    (unsigned int)logBuffer[index].entry.cap_type, /* 5 bit */
                    (unsigned int)logBuffer[index].entry.invocation_tag, /* 19 bit */
                    logBuffer[index].entry.is_fastpath ? "yes" : "no");
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
    fprintf(fd, " %-10s | %-8s | %-20s | %-10s \n",
            "Log ID", "IRQ", "Start Time", "Duration");
    fprintf(fd,
            "-----------------------------------------------------------------------------------------------------------------------------\n");

    while (logBuffer[index].start_time != 0 && (index * sizeof(benchmark_track_kernel_entry_t)) < logSize) {
        if (logBuffer[index].entry.path == Entry_Interrupt) {
            fprintf(fd, " %-10"SEL4_PRIu_word" | %-8u | %-20"PRIu64" | %-10u\n", \
                    index,
                    (unsigned int)logBuffer[index].entry.word, /* 26 bits */
                    logBuffer[index].start_time, /* 64 bit */
                    logBuffer[index].duration); /* 32 bit */
        }
        index++;
    }
}
#endif /* CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES */

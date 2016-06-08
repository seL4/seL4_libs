/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#pragma once
#include <autoconf.h>
#ifdef CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES

#include <stdio.h>
#include <sel4/types.h>
#include <sel4/benchmark_track_types.h>

/* Print out a summary of what has been tracked */
static inline void
seL4_BenchmarkTrackDumpSummary(void)
{
    seL4_Word start_index = 0;
    seL4_Word remaining_entries = 0;
    seL4_Word received_entries = 0;
    seL4_Word syscall_entries = 0;
    seL4_Word interrupt_entries = 0;
    seL4_Word userlevelfault_entries = 0;
    seL4_Word vmfault_entries = 0;

    /* Default driver to use for output now is serial.
     * Change this to use other drivers than serial, i.e ethernet
     */
    FILE *fd = stdout;

    /* IPC buffer is used to hold log data from the kernel */
    benchmark_track_kernel_entry_t *ipcBuffer = (benchmark_track_kernel_entry_t *)
                                                & (seL4_GetIPCBuffer()->msg[0]);

    /* Get the total number of kernel entries between a call to seL4_BenchmarkResetLog and
     * seL4_BenchmarkFinalizeLog. Note, this might be limited by the kernel log size,
     * once it get filled, it stops tracking silently.
     */
    remaining_entries = (seL4_Uint32) seL4_BenchmarkLogSize();

    /* Get number of invocations on each cap */
    fprintf(fd, "---------------------------------------------------------------------------------------------------------\n");
    fprintf(fd, "| The kernel logged %d entries\n", remaining_entries);
    fprintf(fd, "---------------------------------------------------------------------------------------------------------\n");

    /* Copy the kernel logged data to the IPC Buffer, and dump it. This operation will be repeated
     * many times depending on both the kernel log and IPC Buffer sizes.
     */
    while (remaining_entries) {
        received_entries = seL4_BenchmarkDumpLog(start_index, remaining_entries);

        for (int i = 0; i < received_entries; i++) {
            if (ipcBuffer[i].entry.path == Entry_Syscall) {
                syscall_entries++;
            } else if (ipcBuffer[i].entry.path == Entry_Interrupt) {
                interrupt_entries++;
            } else if (ipcBuffer[i].entry.path == Entry_UserLevelFault) {
                userlevelfault_entries++;
            } else if (ipcBuffer[i].entry.path == Entry_VMFault) {
                vmfault_entries++;
            }
        }

        start_index += received_entries;
        remaining_entries -= received_entries;

    };

    fprintf(fd, "Number of system call invocations %d\n", syscall_entries);
    fprintf(fd, "Number of interrupt invocations %d\n", interrupt_entries);
    fprintf(fd, "Number of user-level faults %d\n", userlevelfault_entries);
    fprintf(fd, "Number of VM faults %d\n", vmfault_entries);

}

/* Print out logged system call invocations */
static inline void
seL4_BenchmarkTrackDumpFullSyscallLog(void)
{
    seL4_Word start_index = 0;
    seL4_Word remaining_entries = 0;
    seL4_Word received_entries = 0;
    FILE *fd = stdout;

    benchmark_track_kernel_entry_t *ipcBuffer = (benchmark_track_kernel_entry_t *)
                                                & (seL4_GetIPCBuffer()->msg[0]);

    /* Get details of each system call invocation */
    fprintf(fd, "-----------------------------------------------------------------------------------------------------------------------------\n");
    fprintf(fd, "|     %-15s|     %-15s|     %-15s|     %-15s|     %-15s|     %-15s|     %-15s|\n",
            "Log ID", "System Call ID", "Start Time", "Duration", "Capability Type",
            "Invocation Tag",  "Fastpath?");
    fprintf(fd, "-----------------------------------------------------------------------------------------------------------------------------\n");

    remaining_entries = (seL4_Uint32) seL4_BenchmarkLogSize();

    while (remaining_entries) {
        received_entries = seL4_BenchmarkDumpLog(start_index, remaining_entries);

        for (int i = 0; i < received_entries; i++) {
            if (ipcBuffer[i].entry.path == Entry_Syscall) {
                fprintf(fd, "|     %-15d|     %-15d|     %-15llu|     %-15d|     %-15d|     %-15d|     %-15d|\n",
                        i + start_index,
                        ipcBuffer[i].entry.syscall_no,
                        (uint64_t ) ipcBuffer[i].start_time,
                        ipcBuffer[i].duration,
                        ipcBuffer[i].entry.cap_type,
                        ipcBuffer[i].entry.invocation_tag,
                        ipcBuffer[i].entry.is_fastpath);
            }
        }

        start_index += received_entries;
        remaining_entries -= received_entries;

    };
}

/* Print out logged interrupt invocations */
static inline void
seL4_BenchmarkTrackDumpFullInterruptLog(void)
{
    seL4_Word start_index = 0;
    seL4_Word remaining_entries = 0;
    seL4_Word received_entries = 0;
    FILE *fd = stdout;

    benchmark_track_kernel_entry_t *ipcBuffer = (benchmark_track_kernel_entry_t *)
                                                & (seL4_GetIPCBuffer()->msg[0]);

    /* Get details of each invocation */
    fprintf(fd, "-----------------------------------------------------------------------------------------------------------------------------\n");
    fprintf(fd, "|     %-15s|     %-15s|     %-15s|     %-15s|\n", "Log ID", "IRQ", "Start Time",
            "Duration");
    fprintf(fd, "-----------------------------------------------------------------------------------------------------------------------------\n");

    remaining_entries = (seL4_Uint32) seL4_BenchmarkLogSize();

    while (remaining_entries) {
        received_entries = seL4_BenchmarkDumpLog(start_index, remaining_entries);

        for (int i = 0; i < received_entries; i++) {
            if (ipcBuffer[i].entry.path == Entry_Interrupt) {
                fprintf(fd, "|     %-15d|     %-15d|     %-15llu|     %-15d|\n", \
                        i + start_index,
                        ipcBuffer[i].entry.word,
                        ipcBuffer[i].start_time,
                        ipcBuffer[i].duration);
            }
        }

        start_index += received_entries;
        remaining_entries -= received_entries;

    };

}
#endif /* CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES */

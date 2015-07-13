/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __SEL4_BENCHMARK_H
#define __SEL4_BENCHMARK_H

/* entire ipc buffer except tag register (word 0) */
#define MAX_IPC_BUFFER (1024 - 1)

#include <sel4/simple_types.h>
#include <sel4/printf.h>

// Include seL4_BenchmarkLogSize and seL4_BenchmarkDumpLog.
#include <sel4/arch/syscalls.h>

/**
 * Dump the benchmark log. The kernel must be compiled with CONFIG_BENCHMARK=y,
 * either edit .config manually or add it using "make menuconfig" by selecting
 * "Kernel" -> "seL4 System Parameters" -> "Adds a log buffer to the kernel for instrumentation."
 */
static inline void
seL4_BenchmarkDumpFullLog()
{
    seL4_Uint32 potential_size = seL4_BenchmarkLogSize();

    for (seL4_Uint32 j = 0; j < potential_size; j += MAX_IPC_BUFFER) {
        seL4_Uint32 chunk = potential_size - j;
        seL4_Uint32 requested = chunk > MAX_IPC_BUFFER ? MAX_IPC_BUFFER : chunk;
        seL4_Uint32 recorded = seL4_BenchmarkDumpLog(j, requested);
        for (seL4_Uint32 i = 0; i < recorded; i++) {
            seL4_Printf("%u\t", seL4_GetMR(i));
        }
        seL4_Printf("\n");
        /* we filled the log buffer */
        if (requested != recorded) {
            seL4_Printf("Dumped %u of %u potential logs\n", j + recorded, potential_size);
            return;
        }
    }

    /* logged amount was smaller than log buffer */
    seL4_Printf("Dumped entire log, size %u\n", potential_size);
}

#endif // __SEL4_BENCHMARK_H


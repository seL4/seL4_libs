/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <sel4bench/kernel_logging.h>

#define MAX_IPC_BUFFER (128 - 1)
#define MAX_IPC_LOGS ((MAX_IPC_BUFFER) / sizeof(log_entry_t))

unsigned int kernel_logging_sync_log(log_entry_t log[], unsigned int n) {
#ifdef CONFIG_BENCHMARK
    unsigned int size = seL4_BenchmarkLogSize();
    unsigned int index = 0;

    for (int i = 0; i < size; i += MAX_IPC_LOGS) {
        int num_remaining_entries = size - i;
        int request_size = num_remaining_entries > MAX_IPC_LOGS ? MAX_IPC_LOGS : num_remaining_entries;
        int num_recorded = seL4_BenchmarkDumpLog(i, request_size);
        for (int  j = 0; j < num_recorded; j++) {

            if (index == n) {
                return n;
            }

            int base_index = j * 2;
            seL4_Word key = seL4_GetMR(base_index);
            seL4_Word data = seL4_GetMR(base_index+1);
            log[index] = (log_entry_t) {key, data};
            ++index;
        }
    }

    return index;
#else
    return 0;
#endif
}

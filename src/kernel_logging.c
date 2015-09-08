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

#if CONFIG_MAX_NUM_TRACE_POINTS > 0
#define MAX_IPC_BUFFER (seL4_MsgMaxLength - 1)
#define MAX_IPC_LOGS ((MAX_IPC_BUFFER) / sizeof(kernel_log_entry_t))

unsigned int kernel_logging_sync_log(kernel_log_entry_t log[], unsigned int n) {
    unsigned int size = kernel_logging_log_size();
    unsigned int index = 0;

    for (int i = 0; i < size; i += MAX_IPC_LOGS) {
        int num_remaining_entries = size - i;
        int request_size = num_remaining_entries > MAX_IPC_LOGS ? MAX_IPC_LOGS : num_remaining_entries;
        int num_recorded = kernel_logging_dump_log(i, request_size);
        for (int  j = 0; j < num_recorded; j++) {

            if (index == n) {
                return n;
            }

            int base_index = j * 2;
            seL4_Word key = seL4_GetMR(base_index);
            seL4_Word data = seL4_GetMR(base_index+1);
            log[index] = (kernel_log_entry_t) {key, data};
            ++index;
        }
    }

    return index;
}

#else
unsigned int kernel_logging_sync_log(kernel_log_entry_t log[], unsigned int n) {
    return 0;
}
#endif

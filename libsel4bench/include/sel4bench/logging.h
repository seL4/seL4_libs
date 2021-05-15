/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sel4/types.h>
#include <sel4/constants.h>
#include <sel4bench/kernel_logging.h>

/* Log format matching the one used in the kernel.
 * This will be used as a generic log entry struct.
 */

/* Dynamically expanding array */
typedef struct log_buffer {
    seL4_Word *buffer;
    unsigned int capacity;
    unsigned int length;

    /* Flag set when length == capacity and the most recent
     * call to realloc has failed (indicating a lack of
     * available heap).
     */
    unsigned int full;
} log_buffer_t;

/* Allocates initial memory for the log buffer's internal buffer */
void logging_init_log_buffer(log_buffer_t *log_buffer, unsigned int initial_capacity);

/* Adds a new entry to the log buffer, reallocating memory to expand its buffer if necessary */
void logging_append_log_buffer(log_buffer_t *log_buffer, seL4_Word data);

/* Given an array of log entries and an array of buffers, copies the data field of each array entry
 * into a buffer whose position in the array of buffers corresponds to the log entry's key field.
 */
void logging_separate_log(kernel_log_entry_t *logs, unsigned int num_logs, log_buffer_t *buffers, unsigned int num_buffers);

/* Sorts an array of logs in place, in ascending order of key.
 * Not necessarily a stable sort.
 */
void logging_sort_log(kernel_log_entry_t *logs, unsigned int num_logs);

/* Sorts an array of logs in place, in ascending order of key.
 * Guaranteed to be stable.
 */
void logging_stable_sort_log(kernel_log_entry_t *logs, unsigned int num_logs);

/* Given a sorted array of logs, for each distinct key, records the offset in the now sorted array
 * containing the first occurence of that key, and the number of elements in the array with that key.
 */
void logging_group_log_by_key(kernel_log_entry_t *logs, unsigned int num_logs,
                              unsigned int *sizes, unsigned int *offsets,
                              unsigned int max_groups);

/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#include <sel4bench/logging.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>

void
logging_init_log_buffer(log_buffer_t *log_buffer, unsigned int initial_capacity)
{
    log_buffer->capacity = initial_capacity;
    log_buffer->length = 0;
    log_buffer->buffer = (seL4_Word*)malloc(initial_capacity * sizeof(seL4_Word));
    log_buffer->full = 0;
    assert(log_buffer->buffer);
}

static void
expand_log_buffer(log_buffer_t *log_buffer)
{
    unsigned int new_capacity = log_buffer->capacity * 2;
    seL4_Word* new_buffer = (seL4_Word*)realloc(log_buffer->buffer, new_capacity * sizeof(seL4_Word));
    if (new_buffer == NULL) {
        log_buffer->full = 1;
    } else {
        log_buffer->buffer = new_buffer;
        log_buffer->capacity = new_capacity;

    }
}

void
logging_append_log_buffer(log_buffer_t *log_buffer, seL4_Word data)
{
    if (log_buffer->length == log_buffer->capacity) {
        expand_log_buffer(log_buffer);
    }
    if (!log_buffer->full) {
        log_buffer->buffer[log_buffer->length] = data;
        ++log_buffer->length;
    }
}

void
logging_separate_log(kernel_log_entry_t *logs, unsigned int num_logs, log_buffer_t *buffers, unsigned int num_buffers)
{
    for (int i = 0; i < num_logs; ++i) {
        kernel_log_entry_t *entry = &logs[i];
        seL4_Word key = kernel_logging_entry_get_key(entry);
        seL4_Word data = kernel_logging_entry_get_data(entry);
        if (key < num_buffers) {
            logging_append_log_buffer(&buffers[key], data);
        }
    }
}

static int
log_compare(const void *a, const void *b)
{
    return kernel_logging_entry_get_key((kernel_log_entry_t*)a) -
           kernel_logging_entry_get_key((kernel_log_entry_t*)b);
}

void
logging_sort_log(kernel_log_entry_t *logs, unsigned int num_logs)
{
    qsort(logs, num_logs, sizeof(kernel_log_entry_t), log_compare);
}

void
logging_stable_sort_log(kernel_log_entry_t *logs, unsigned int num_logs)
{
    /* Modify the keys so sorting preserves order within a single key */
    for (int i = 0; i < num_logs; ++i) {
        kernel_log_entry_t *entry = &logs[i];
        seL4_Word key = kernel_logging_entry_get_key(entry);
        key = key * num_logs + i;
    }

    logging_sort_log(logs, num_logs);

    /* Restore the original key values */
    for (int i = 0; i < num_logs; ++i) {
        kernel_log_entry_t *entry = &logs[i];
        seL4_Word key = kernel_logging_entry_get_key(entry);
        key = key / num_logs;
    }
}

void
logging_group_log_by_key(kernel_log_entry_t *logs, unsigned int num_logs,
                         unsigned int *sizes, unsigned int *offsets,
                         unsigned int max_groups)
{

    int index = 0;
    for (int i = 0; i < max_groups; ++i) {
        offsets[i] = index;
        while (index < num_logs && kernel_logging_entry_get_key(&logs[index]) == i) {
            ++index;
        }
        sizes[i] = index - offsets[i];
    }
}

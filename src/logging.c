/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <sel4bench/logging.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>

void logging_init_log_buffer(log_buffer_t *log_buffer, unsigned int initial_capacity) {
    log_buffer->capacity = initial_capacity;
    log_buffer->length = 0;
    log_buffer->buffer = (seL4_Word*)malloc(initial_capacity * sizeof(seL4_Word));
    log_buffer->full = 0;
    assert(log_buffer->buffer);
}

static void expand_log_buffer(log_buffer_t *log_buffer) {
    unsigned int new_capacity = log_buffer->capacity * 2;
    seL4_Word* new_buffer = (seL4_Word*)realloc(log_buffer->buffer, new_capacity * sizeof(seL4_Word));
    if (new_buffer == NULL) {
        log_buffer->full = 1;
    } else {
        log_buffer->buffer = new_buffer;
        log_buffer->capacity = new_capacity;

    }
}

void logging_append_log_buffer(log_buffer_t *log_buffer, seL4_Word data) {
    if (log_buffer->length == log_buffer->capacity) {
        expand_log_buffer(log_buffer);
    }
    if (!log_buffer->full) {
        log_buffer->buffer[log_buffer->length] = data;
        ++log_buffer->length;
    }
}

void logging_separate_log(log_entry_t *logs, unsigned int num_logs, log_buffer_t *buffers, unsigned int num_buffers) {
    for (int i = 0; i < num_logs; ++i) {
        log_entry_t *entry = &logs[i];
        if (entry->key < num_buffers) {
            logging_append_log_buffer(&buffers[entry->key], entry->data);
        }
    }
}

static void
swap(log_entry_t *logs, int i, int j) {
    log_entry_t tmp = logs[i];
    logs[i] = logs[j];
    logs[j] = tmp;
}

static int
partition(log_entry_t *logs, int lo, int hi) {

    int pivot;
    int i = lo - 1;
    int j = hi + 1;

    /* We will use the middle element as the pivot.
     * Swap it to the start of the range and use Hoare partitioning. */
    swap(logs, lo, (hi + lo) / 2);

    pivot = logs[lo].key;

    while (1) {
        do {
            ++i;
        } while (logs[i].key < pivot);
        do {
            --j;
        } while (logs[j].key > pivot);
        if (i < j) {
            swap(logs, i, j);
        } else {
            return j;
        }
    }
}

static void
quicksort(log_entry_t *logs, int lo, int hi) {
    if (lo < hi) {
        int pivot_index = partition(logs, lo, hi);
        quicksort(logs, lo, pivot_index);
        quicksort(logs, pivot_index + 1, hi);
    }
}

void logging_sort_log(log_entry_t *logs, unsigned int num_logs) {
    quicksort(logs, 0, num_logs - 1);
}

void logging_stable_sort_log(log_entry_t *logs, unsigned int num_logs) {
    /* Modify the keys so sorting preserves order within a single key */
    for (int i = 0; i < num_logs; ++i) {
        log_entry_t *entry = &logs[i];
        entry->key = entry->key * num_logs + i;
    }

    logging_sort_log(logs, num_logs);

    /* Restore the original key values */
    for (int i = 0; i < num_logs; ++i) {
        log_entry_t *entry = &logs[i];
        entry->key = entry->key / num_logs;
    }
}

void
logging_group_log_by_key(log_entry_t *logs, unsigned int num_logs,
                         unsigned int *sizes, unsigned int *offsets,
                         unsigned int max_groups)
{

    int index = 0;
    for (int i = 0; i < max_groups; ++i) {
        offsets[i] = index;
        while (index < num_logs && logs[index].key == i) {
            ++index;
        }
        sizes[i] = index - offsets[i];
    }
}

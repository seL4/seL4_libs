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

void logging_separate_log(seL4_LogEntry *logs, unsigned int num_logs, log_buffer_t *buffers, unsigned int num_buffers) {
    for (int i = 0; i < num_logs; ++i) {
        seL4_LogEntry *entry = &logs[i];
        if (entry->key < num_buffers) {
            logging_append_log_buffer(&buffers[entry->key], entry->data);
        }
    }
}

static int
log_compare(const void *a, const void *b) {
    return ((seL4_LogEntry*)a)->key - ((seL4_LogEntry*)b)->key;
}

void logging_sort_log(seL4_LogEntry *logs, unsigned int num_logs) {
    qsort(logs, num_logs, sizeof(seL4_LogEntry), log_compare);
}

void logging_stable_sort_log(seL4_LogEntry *logs, unsigned int num_logs) {
    /* Modify the keys so sorting preserves order within a single key */
    for (int i = 0; i < num_logs; ++i) {
        seL4_LogEntry *entry = &logs[i];
        entry->key = entry->key * num_logs + i;
    }

    logging_sort_log(logs, num_logs);

    /* Restore the original key values */
    for (int i = 0; i < num_logs; ++i) {
        seL4_LogEntry *entry = &logs[i];
        entry->key = entry->key / num_logs;
    }
}

void
logging_group_log_by_key(seL4_LogEntry *logs, unsigned int num_logs,
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

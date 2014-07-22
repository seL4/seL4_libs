/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _SYNC_MUTEX_H_
#define _SYNC_MUTEX_H_

#include <sel4/sel4.h>

typedef struct {
    seL4_CPtr aep;
} sync_mutex_t;

int sync_mutex_init(sync_mutex_t *mutex, seL4_CPtr aep);
int sync_mutex_lock(sync_mutex_t *mutex);
int sync_mutex_unlock(sync_mutex_t *mutex);
int sync_mutex_destroy(sync_mutex_t *mutex);

#endif

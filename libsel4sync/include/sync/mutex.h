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
#include <vka/vka.h>
#include <sync/bin_sem.h>

typedef sync_bin_sem_t sync_mutex_t;

static inline int sync_mutex_init(sync_mutex_t *mutex, seL4_CPtr notification) {
    return sync_bin_sem_init(mutex, notification);
}

static inline int sync_mutex_lock(sync_mutex_t *mutex) {
    return sync_bin_sem_wait(mutex);
}

static inline int sync_mutex_unlock(sync_mutex_t *mutex) {
    return sync_bin_sem_post(mutex);
}

static inline int sync_mutex_new(vka_t *vka, sync_mutex_t *mutex) {
    return sync_bin_sem_new(vka, mutex);
}

static inline int sync_mutex_destroy(vka_t *vka, sync_mutex_t *mutex) {
    return sync_bin_sem_destroy(vka, mutex);
}

#endif

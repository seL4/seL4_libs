/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sel4/sel4.h>
#include <vka/vka.h>
#include <sync/bin_sem.h>

typedef sync_bin_sem_t sync_mutex_t;

/* Initialise an unmanaged mutex with a notification object
 * @param sem           A mutex object to be initialised.
 * @param notification  A notification object to use for the lock.
 * @return              0 on success, an error code on failure. */
static inline int sync_mutex_init(sync_mutex_t *mutex, seL4_CPtr notification) {
    return sync_bin_sem_init(mutex, notification, 1);
}

/* Acquire a mutex
 * @param mutex         An initialised mutex to acquire.
 * @return              0 on success, an error code on failure. */
static inline int sync_mutex_lock(sync_mutex_t *mutex) {
    return sync_bin_sem_wait(mutex);
}

/* Release a mutex
 * @param mutex         An initialised mutex to release.
 * @return              0 on success, an error code on failure. */
static inline int sync_mutex_unlock(sync_mutex_t *mutex) {
    return sync_bin_sem_post(mutex);
}

/* Allocate and initialise a managed mutex
 * @param vka           A VKA instance used to allocate a notification object.
 * @param mutex         A mutex object to initialise.
 * @return              0 on success, an error code on failure. */
static inline int sync_mutex_new(vka_t *vka, sync_mutex_t *mutex) {
    return sync_bin_sem_new(vka, mutex, 1);
}

/* Deallocate a managed mutex (do not use with sync_mutex_init)
 * @param vka           A VKA instance used to deallocate the notification object.
 * @param mutex         A mutex object initialised by sync_mutex_new.
 * @return              0 on success, an error code on failure. */
static inline int sync_mutex_destroy(vka_t *vka, sync_mutex_t *mutex) {
    return sync_bin_sem_destroy(vka, mutex);
}


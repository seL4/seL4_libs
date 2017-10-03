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

/* An implementation of recursive mutexes. Assumptions are similar to pthread
 * recursive mutexes. That is, the thread that locked the mutex needs to be the
 * one who unlocks the mutex, mutexes must be unlocked as many times as they
 * are locked, etc.
 *
 * Note that the address of your IPC buffer is used as a thread ID so if you
 * have a situation where threads share an IPC buffer or do not have a valid
 * IPC buffer, these locks will not work for you.
 */

#pragma once

#include <sel4/sel4.h>
#include <vka/vka.h>
#include <vka/object.h>

/* This struct is intended to be opaque, but is left here so you can
 * stack-allocate mutexes. Callers should not touch any of its members.
 */
typedef struct {
    vka_object_t notification;
    void *owner;
    unsigned int held;
} sync_recursive_mutex_t;

/* Initialise an unmanaged recursive mutex with a notification object
 * @param mutex         A recursive mutex object to be initialised.
 * @param notification  A notification object to use for the lock.
 * @return              0 on success, an error code on failure. */
int sync_recursive_mutex_init(sync_recursive_mutex_t *mutex, seL4_CPtr notification);

/* Acquire a recursive mutex
 * @param mutex         An initialised recursive mutex to acquire.
 * @return              0 on success, an error code on failure. */
int sync_recursive_mutex_lock(sync_recursive_mutex_t *mutex);

/* Release a recursive mutex
 * @param mutex         An initialised recursive mutex to release.
 * @return              0 on success, an error code on failure. */
int sync_recursive_mutex_unlock(sync_recursive_mutex_t *mutex);

/* Allocate and initialise a managed recursive mutex
 * @param vka           A VKA instance used to allocate a notification object.
 * @param mutex         A recursive mutex object to initialise.
 * @return              0 on success, an error code on failure. */
int sync_recursive_mutex_new(vka_t *vka, sync_recursive_mutex_t *mutex);

/* Deallocate a managed recursive mutex (do not use with sync_mutex_init)
 * @param vka           A VKA instance used to deallocate the notification object.
 * @param mutex         A recursive mutex object initialised by sync_recursive_mutex_new.
 * @return              0 on success, an error code on failure. */
int sync_recursive_mutex_destroy(vka_t *vka, sync_recursive_mutex_t *mutex);


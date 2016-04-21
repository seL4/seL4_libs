/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
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

#ifndef _SYNC_RECURSIVE_MUTEX_H_
#define _SYNC_RECURSIVE_MUTEX_H_

#include <sel4/sel4.h>

/* This struct is intended to be opaque, but is left here so you can
 * stack-allocate mutexes. Callers should not touch any of its members.
 */
typedef struct {
    seL4_CPtr notification;
    void *owner;
    unsigned int held;
} sync_recursive_mutex_t;

int sync_recursive_mutex_init(sync_recursive_mutex_t *mutex, seL4_CPtr notification);
int sync_recursive_mutex_lock(sync_recursive_mutex_t *mutex);
int sync_recursive_mutex_unlock(sync_recursive_mutex_t *mutex);
int sync_recursive_mutex_destroy(sync_recursive_mutex_t *mutex);

#endif

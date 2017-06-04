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

#include <autoconf.h>
#include <sync/recursive_mutex.h>
#include <stddef.h>
#include <assert.h>
#include <limits.h>

#include <sel4/sel4.h>

static void *thread_id(void) {
    return (void*)seL4_GetIPCBuffer();
}

int sync_recursive_mutex_init(sync_recursive_mutex_t *mutex, seL4_CPtr notification) {
    if (mutex == NULL) {
        ZF_LOGE("Mutex passed to sync_recursive_mutex_init is NULL");
        return -1;
    }
#ifdef CONFIG_DEBUG_BUILD
    /* Check the cap actually is a notification. */
    assert(seL4_DebugCapIdentify(notification) == 6);
#endif

    mutex->notification.cptr = notification;
    mutex->owner = NULL;
    mutex->held = 0;

    /* Prime the endpoint. */
    seL4_Signal(mutex->notification.cptr);
    return 0;
}

int sync_recursive_mutex_lock(sync_recursive_mutex_t *mutex) {
    if (mutex == NULL) {
        ZF_LOGE("Mutex passed to sync_recursive_mutex_lock is NULL");
        return -1;
    }
    if (thread_id() != mutex->owner) {
        /* We don't already have the mutex. */
        seL4_Wait(mutex->notification.cptr, NULL);
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
        assert(mutex->owner == NULL);
        mutex->owner = thread_id();
        assert(mutex->held == 0);
    }
    if (mutex->held == UINT_MAX) {
        /* We would overflow if we re-acquired the mutex. Note that we can only
         * be in this branch if we already held the mutex before entering this
         * function, so we don't need to release the mutex here.
         */
        return -1;
    }
    mutex->held++;
    return 0;
}

int sync_recursive_mutex_unlock(sync_recursive_mutex_t *mutex) {
    if (mutex == NULL) {
        ZF_LOGE("Mutex passed to sync_recursive_mutex_lock is NULL");
        return -1;
    }
    assert(mutex->owner == thread_id());
    assert(mutex->held > 0);
    mutex->held--;
    if (mutex->held == 0) {
        /* This was the outermost lock we held. Wake the next person up. */
        __atomic_store_n(&mutex->owner, NULL, __ATOMIC_RELEASE);
        seL4_Signal(mutex->notification.cptr);
    }
    return 0;
}

int sync_recursive_mutex_new(vka_t *vka, sync_recursive_mutex_t *mutex) {
    int error = vka_alloc_notification(vka, &(mutex->notification));

    if (error != 0) {
        return error;
    } else {
        return sync_recursive_mutex_init(mutex, mutex->notification.cptr);
    }
}

int sync_recursive_mutex_destroy(vka_t *vka, sync_recursive_mutex_t *mutex) {
    vka_free_object(vka, &(mutex->notification));
    return 0;
}

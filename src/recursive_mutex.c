/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <sync/recursive_mutex.h>
#include <stddef.h>
#include <assert.h>
#include <limits.h>

#include <sel4/sel4.h>

static void *thread_id(void) {
    return (void*)seL4_GetIPCBuffer();
}

int sync_recursive_mutex_init(sync_recursive_mutex_t *mutex, seL4_CPtr aep) {
    assert(mutex != NULL);
#ifdef SEL4_DEBUG_KERNEL
    /* Check the cap actually is an AEP. */
    assert(seL4_DebugCapIdentify(aep) == 3);
#endif

    mutex->aep = aep;
    mutex->owner = NULL;
    mutex->held = 0;

    /* Prime the endpoint. */
    seL4_Notify(mutex->aep, 1);
    return 0;
}

int sync_recursive_mutex_lock(sync_recursive_mutex_t *mutex) {
    assert(mutex != NULL);
    if (thread_id() != mutex->owner) {
        /* We don't already have the mutex. */
        (void)seL4_Wait(mutex->aep, NULL);
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
    assert(mutex != NULL);
    assert(mutex->owner == thread_id());
    assert(mutex->held > 0);
    mutex->held--;
    if (mutex->held == 0) {
        /* This was the outermost lock we held. Wake the next person up. */
        mutex->owner = NULL;
        seL4_Notify(mutex->aep, 1);
    }
    return 0;
}

int sync_recursive_mutex_destroy(sync_recursive_mutex_t *mutex) {
    /* Nothing to be done. */
    return 0;
}

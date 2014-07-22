/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <sync/mutex.h>
#include <stddef.h>
#include <assert.h>

#include <sel4/sel4.h>

int sync_mutex_init(sync_mutex_t *mutex, seL4_CPtr aep) {
    assert(mutex != NULL);
#ifdef SEL4_DEBUG_KERNEL
    /* Check the cap actually is an AEP. */
    assert(seL4_DebugCapIdentify(aep) == 6);
#endif

    mutex->aep = aep;

    /* Prime the endpoint. */
    seL4_Notify(mutex->aep, 1);
    return 0;
}

int sync_mutex_lock(sync_mutex_t *mutex) {
    assert(mutex != NULL);
    (void)seL4_Wait(mutex->aep, NULL);
    __sync_synchronize();
    return 0;
}

int sync_mutex_unlock(sync_mutex_t *mutex) {
    assert(mutex != NULL);
    /* Wake the next person up. */
    __sync_synchronize();
    seL4_Notify(mutex->aep, 1);
    return 0;
}

int sync_mutex_destroy(sync_mutex_t *mutex) {
    /* Nothing to be done. */
    return 0;
}

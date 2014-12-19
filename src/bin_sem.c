/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <sync/bin_sem.h>
#include <sync/atomic.h>
#include <stddef.h>
#include <assert.h>

int sync_bin_sem_init(sync_bin_sem_t *sem, seL4_CPtr aep) {
    assert(sem != NULL);
#ifdef SEL4_DEBUG_KERNEL
    /* Check the cap actually is an AEP. */
    assert(seL4_DebugCapIdentify(aep) == 6);
#endif

    sem->aep = aep;
    sem->value = 1;
    return 0;
}

int sync_bin_sem_wait(sync_bin_sem_t *sem) {
    assert(sem != NULL);
    int value = sync_atomic_decrement(&sem->value);
    if (value < 0) {
        seL4_Wait(sem->aep, NULL);
    }
    __sync_synchronize();
    return 0;
}

int sync_bin_sem_post(sync_bin_sem_t *sem) {
    assert(sem != NULL);
    __sync_synchronize();
    int value = sync_atomic_increment(&sem->value);
    assert(sem->value <= 1);
    if (value <= 0) {
        seL4_Notify(sem->aep, 0);
    }
    return 0;
}

int sync_bin_sem_destroy(sync_bin_sem_t *sem) {
    assert(sem != NULL);
    return 0;
}

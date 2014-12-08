/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <sync/sem.h>
#include <sync/atomic.h>
#include <stddef.h>
#include <assert.h>

int sync_sem_init(sync_sem_t *sem, seL4_CPtr ep, int value) {
    assert(sem != NULL);
#ifdef SEL4_DEBUG_KERNEL
    /* Check the cap actually is an EP. */
    assert(seL4_DebugCapIdentify(ep) == 4);
#endif

    sem->ep = ep;
    sem->value = value;
    return 0;
}

int sync_sem_wait(sync_sem_t *sem) {
    assert(sem != NULL);
    int value = sync_atomic_decrement(&sem->value);
    if (value < 0) {
        seL4_Wait(sem->ep, NULL);
    }
    __sync_synchronize();
    return 0;
}

int sync_sem_trywait(sync_sem_t *sem) {
    assert(sem != NULL);
    int val = sem->value;
    while (val > 0) {
        int oldval = sync_atomic_compare_and_swap(&sem->value, val, val - 1);
        if (oldval == val) {
            /* We got it! */
            __sync_synchronize();
            return 0;
        }
        /* We didn't get it. */
        val = oldval;
    }
    /* The semaphore is empty. */
    return -1;
}

int sync_sem_post(sync_sem_t *sem) {
    assert(sem != NULL);
    __sync_synchronize();
    int value = sync_atomic_increment(&sem->value);
    if (value <= 0) {
        seL4_Notify(sem->ep, 0);
    }
    return 0;
}

int sync_sem_destroy(sync_sem_t *sem) {
    assert(sem != NULL);
    return 0;
}

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _SYNC_SEM_H_
#define _SYNC_SEM_H_

#include <assert.h>
#include <sel4/sel4.h>
#include <stddef.h>
#include <sync/sem-bare.h>

typedef struct {
    seL4_CPtr ep;
    volatile int value;
} sync_sem_t;

static inline int sync_sem_init(sync_sem_t *sem, seL4_CPtr ep, int value) {
    assert(sem != NULL);
#ifdef SEL4_DEBUG_KERNEL
    /* Check the cap actually is an EP. */
    assert(seL4_DebugCapIdentify(ep) == 4);
#endif

    sem->ep = ep;
    sem->value = value;
    return 0;
}

static inline int sync_sem_wait(sync_sem_t *sem) {
    assert(sem != NULL);
    return sync_sem_bare_wait(sem->ep, &sem->value);
}

static inline int sync_sem_trywait(sync_sem_t *sem) {
    assert(sem != NULL);
    return sync_sem_bare_trywait(sem->ep, &sem->value);
}

static inline int sync_sem_post(sync_sem_t *sem) {
    assert(sem != NULL);
    return sync_sem_bare_post(sem->ep, &sem->value);
}

static inline int sync_sem_destroy(sync_sem_t *sem) {
    assert(sem != NULL);
    return 0;
}

#endif

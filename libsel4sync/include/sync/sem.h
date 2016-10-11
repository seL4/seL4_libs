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
#include <vka/vka.h>
#include <vka/object.h>
#include <stddef.h>
#include <sync/sem-bare.h>

typedef struct {
    vka_object_t ep;
    volatile int value;
} sync_sem_t;

static inline int sync_sem_init(sync_sem_t *sem, seL4_CPtr ep, int value) {
    assert(sem != NULL);
#ifdef SEL4_DEBUG_KERNEL
    /* Check the cap actually is an EP. */
    assert(seL4_DebugCapIdentify(ep) == 4);
#endif

    sem->ep.cptr = ep;
    sem->value = value;
    return 0;
}

static inline int sync_sem_wait(sync_sem_t *sem) {
    assert(sem != NULL);
    return sync_sem_bare_wait(sem->ep.cptr, &sem->value);
}

static inline int sync_sem_trywait(sync_sem_t *sem) {
    assert(sem != NULL);
    return sync_sem_bare_trywait(sem->ep.cptr, &sem->value);
}

static inline int sync_sem_post(sync_sem_t *sem) {
    assert(sem != NULL);
    return sync_sem_bare_post(sem->ep.cptr, &sem->value);
}

static inline int sync_sem_new(vka_t *vka, sync_sem_t *sem, int value) {
    int error = vka_alloc_endpoint(vka, &(sem->ep));
    
    if (error != 0) {
        return error;
    } else {
        return sync_sem_init(sem, sem->ep.cptr, value);
    }
}

static inline int sync_sem_destroy(vka_t *vka, sync_sem_t *sem) {
    assert(sem != NULL);
    vka_free_object(vka, &(sem->ep));
    return 0;
}

#endif

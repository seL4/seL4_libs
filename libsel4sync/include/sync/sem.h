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

#pragma once

#include <autoconf.h>
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

/* Initialise an unmanaged semaphore with an endpoint object
 * @param sem           A semaphore object to be initialised.
 * @param notification  An endpoint to use for the lock.
 * @param value         An initial value for the semaphore.
 * @return              0 on success, an error code on failure. */
static inline int sync_sem_init(sync_sem_t *sem, seL4_CPtr ep, int value) {
    if (sem == NULL) {
        ZF_LOGE("Semaphore passed to sync_sem_init was NULL");
        return -1;
    }
#ifdef CONFIG_DEBUG_BUILD
    /* Check the cap actually is an EP. */
    assert(seL4_DebugCapIdentify(ep) == 4);
#endif

    sem->ep.cptr = ep;
    sem->value = value;
    return 0;
}

/* Wait on a semaphore
 * @param sem           An initialised semaphore to acquire.
 * @return              0 on success, an error code on failure. */
static inline int sync_sem_wait(sync_sem_t *sem) {
    if (sem == NULL) {
        ZF_LOGE("Semaphore passed to sync_sem_wait was NULL");
        return -1;
    }
    return sync_sem_bare_wait(sem->ep.cptr, &sem->value);
}

/* Try to wait on the semaphore without waiting on the endpoint
 * i.e. check the semaphore value in a loop
 * @param sem           An initialised semaphore to acquire.
 * @return              0 on success, an error code on failure. */
static inline int sync_sem_trywait(sync_sem_t *sem) {
    if (sem == NULL) {
        ZF_LOGE("Semaphore passed to sync_sem_trywait was NULL");
        return -1;
    }
    return sync_sem_bare_trywait(sem->ep.cptr, &sem->value);
}

/* Signal a binary semaphore
 * @param sem           An initialised semaphore to release.
 * @return              0 on success, an error code on failure. */
static inline int sync_sem_post(sync_sem_t *sem) {
    if (sem == NULL) {
        ZF_LOGE("Semaphore passed to sync_sem_post was NULL");
        return -1;
    }
    return sync_sem_bare_post(sem->ep.cptr, &sem->value);
}

/* Allocate and initialise a managed semaphore
 * @param vka           A VKA instance used to allocate an endpoint.
 * @param sem           A semaphore object to initialise.
 * @param value         An initial value for the semaphore.
 * @return              0 on success, an error code on failure. */
static inline int sync_sem_new(vka_t *vka, sync_sem_t *sem, int value) {
    if (sem == NULL) {
        ZF_LOGE("Semaphore passed to sync_sem_new was NULL");
        return -1;
    }
    int error = vka_alloc_endpoint(vka, &(sem->ep));

    if (error != 0) {
        return error;
    } else {
        return sync_sem_init(sem, sem->ep.cptr, value);
    }
}

/* Deallocate a managed semaphore (do not use with sync_sem_init)
 * @param vka           A VKA instance used to deallocate the endpoint.
 * @param sem           A semaphore object initialised by sync_sem_new.
 * @return              0 on success, an error code on failure. */
static inline int sync_sem_destroy(vka_t *vka, sync_sem_t *sem) {
    if (sem == NULL) {
        ZF_LOGE("Semaphore passed to sync_sem_destroy was NULL");
        return -1;
    }
    vka_free_object(vka, &(sem->ep));
    return 0;
}


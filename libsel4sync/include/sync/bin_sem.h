/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _SYNC_BIN_SEM_H_
#define _SYNC_BIN_SEM_H_

#include <assert.h>
#include <sel4/sel4.h>
#include <vka/vka.h>
#include <vka/object.h>
#include <stddef.h>
#include <sync/bin_sem_bare.h>

typedef struct {
    vka_object_t notification;
    volatile int value;
} sync_bin_sem_t;

static inline int sync_bin_sem_init(sync_bin_sem_t *sem, seL4_CPtr notification) {
    assert(sem != NULL);
#ifdef SEL4_DEBUG_KERNEL
    /* Check the cap actually is a notification. */
    assert(seL4_DebugCapIdentify(notification) == 6);
#endif

    sem->notification.cptr = notification;
    sem->value = 1;
    return 0;
}

static inline int sync_bin_sem_wait(sync_bin_sem_t *sem) {
    assert(sem != NULL);
    return sync_bin_sem_bare_wait(sem->notification.cptr, &sem->value);
}

static inline int sync_bin_sem_post(sync_bin_sem_t *sem) {
    assert(sem != NULL);
    return sync_bin_sem_bare_post(sem->notification.cptr, &sem->value);
}

static inline int sync_bin_sem_new(vka_t *vka, sync_bin_sem_t *sem) {
    int error = vka_alloc_notification(vka, &(sem->notification));
    
    if (error != 0) {
        return error;
    } else {
        return sync_bin_sem_init(sem, sem->notification.cptr);
    }
}

static inline int sync_bin_sem_destroy(vka_t *vka, sync_bin_sem_t *sem) {
    assert(sem != NULL);
    vka_free_object(vka, &(sem->notification));
    return 0;
}

#endif

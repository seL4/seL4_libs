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

#include <sel4/sel4.h>

typedef struct {
    seL4_CPtr ep;
    volatile int value;
} sync_sem_t;

int sync_sem_init(sync_sem_t *sem, seL4_CPtr ep, int value);
int sync_sem_wait(sync_sem_t *sem);
int sync_sem_trywait(sync_sem_t *sem);
int sync_sem_post(sync_sem_t *sem);
int sync_sem_destroy(sync_sem_t *sem);

#endif

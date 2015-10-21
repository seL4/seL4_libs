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

#include <sel4/sel4.h>

typedef struct {
    seL4_CPtr aep;
    volatile int value;
} sync_bin_sem_t;

int sync_bin_sem_init(sync_bin_sem_t *sem, seL4_CPtr aep);
int sync_bin_sem_wait(sync_bin_sem_t *sem);
int sync_bin_sem_post(sync_bin_sem_t *sem);
int sync_bin_sem_destroy(sync_bin_sem_t *sem);

#endif

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _SYNC_SEM_BARE_H_
#define _SYNC_SEM_BARE_H_

/* An unmanaged semaphore; i.e. the caller stores the state related to the
 * semaphore itself. This can be useful in scenarios such as CAmkES, where
 * immediate concurrency means we have a race on initialising a managed
 * semaphore.
 */

#include <sel4/sel4.h>

int sync_sem_bare_wait(seL4_CPtr ep, volatile int *value);
int sync_sem_bare_post(seL4_CPtr ep, volatile int *value);

#endif

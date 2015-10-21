/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _SYNC_SPINLOCK_H_
#define _SYNC_SPINLOCK_H_

/* Interface for spin locks. Note, the underlying implementation is not
 * seL4-specific. Spin locks should not be used when you have threads of
 * different priorities.
 */

typedef int sync_spinlock_t;

int sync_spinlock_init(sync_spinlock_t *lock);
int sync_spinlock_lock(sync_spinlock_t *lock);
int sync_spinlock_trylock(sync_spinlock_t *lock);
int sync_spinlock_unlock(sync_spinlock_t *lock);
int sync_spinlock_destroy(sync_spinlock_t *lock);

#endif

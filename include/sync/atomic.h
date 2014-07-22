/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _SYNC_ATOMIC_H_
#define _SYNC_ATOMIC_H_

/* Atomically increment an integer and return its new value. */
int sync_atomic_increment(volatile int *x);

/* Atomically decrement an integer and return its new value. */
int sync_atomic_decrement(volatile int *x);

/* Atomic CAS. Returns the old value. */
int sync_atomic_compare_and_swap(volatile int *x, int oldval, int newval);

#endif

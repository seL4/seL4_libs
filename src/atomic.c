/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <sync/atomic.h>

int sync_atomic_increment(volatile int *x) {
    return __sync_add_and_fetch(x, 1);
}

int sync_atomic_decrement(volatile int *x) {
    return __sync_sub_and_fetch(x, 1);
}

int sync_atomic_compare_and_swap(volatile int *x, int oldval, int newval) {
    return __sync_val_compare_and_swap(x, oldval, newval);
}

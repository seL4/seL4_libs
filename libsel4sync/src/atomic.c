/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <sync/atomic.h>

int sync_atomic_increment(volatile int *x, int memorder) {
    return __atomic_add_fetch(x, 1, memorder);
}

int sync_atomic_decrement(volatile int *x, int memorder) {
    return __atomic_sub_fetch(x, 1, memorder);
}

int sync_atomic_increment_safe(volatile int *x, int *oldval, int success_memorder) {
    assert(x != NULL);
    assert(oldval != NULL);
    do {
        *oldval = *x;
        if (*oldval == INT_MAX) {
            /* We would overflow */
            return -1;
        }
    } while (!__atomic_compare_exchange_n(x, oldval, *oldval + 1, 1, success_memorder, __ATOMIC_RELAXED));
    return 0;
}

int sync_atomic_decrement_safe(volatile int *x, int *oldval, int success_memorder) {
    assert(x != NULL);
    assert(oldval != NULL);
    do {
        *oldval = *x;
        if (*oldval == INT_MIN) {
            /* We would overflow */
            return -1;
        }
    } while (!__atomic_compare_exchange_n(x, oldval, *oldval - 1, 1, success_memorder, __ATOMIC_RELAXED));
    return 0;
}

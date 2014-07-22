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
#include <sync/spinlock.h>
#include <utils/util.h>

int sync_spinlock_init(sync_spinlock_t *lock) {
    *lock = 0;
    return 0;
}

int sync_spinlock_lock(sync_spinlock_t *lock) {
    while (!__sync_bool_compare_and_swap(lock, 0, 1));
    return 0;
}

int sync_spinlock_trylock(sync_spinlock_t *lock) {
    return !__sync_bool_compare_and_swap(lock, 0, 1);
}

int sync_spinlock_unlock(sync_spinlock_t *lock) {
    int result UNUSED = __sync_bool_compare_and_swap(lock, 1, 0);
    /* This should have succeeded because we should have been the (only) holder
     * of the lock.
     */
    assert(result == 1);
    return 0;
}

int sync_spinlock_destroy(sync_spinlock_t *lock) {
    /* Nothing required. */
    return 0;
}

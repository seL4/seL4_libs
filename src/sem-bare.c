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
#include <stdlib.h>
#include <sel4/sel4.h>
#include <sync/sem-bare.h>
#include <sync/atomic.h>

int sync_sem_bare_wait(seL4_CPtr ep, volatile int *value) {
#ifdef SEL4_DEBUG_KERNEL
    /* Check the cap actually is an EP. */
    assert(seL4_DebugCapIdentify(ep) == 4);
#endif
    assert(value != NULL);
    int v = sync_atomic_decrement(value);
    if (v < 0) {
        seL4_Wait(ep, NULL);
    }
    return 0;
}

int sync_sem_bare_post(seL4_CPtr ep, volatile int *value) {
#ifdef SEL4_DEBUG_KERNEL
    /* Check the cap actually is an EP. */
    assert(seL4_DebugCapIdentify(ep) == 4);
#endif
    assert(value != NULL);
    int v = sync_atomic_increment(value);
    if (v <= 0) {
        seL4_Notify(ep, 0);
    }
    return 0;
}

/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 *  @LICENSE(DATA61_BSD)
 */

#include <sel4debug/stack.h>
#include <utils/stack.h>

/* A simple spin lock that we will use to protect usage of the emergency stack.
 * Note that this is not advisable in the presence of threads of different
 * priorities, but we assume callers are only invoking this as a last ditch
 * debugging effort anyway. If you use this functionality with threads of
 * different priorities you will likely livelock your system.
 */
static int stack_lock;

/* An emergency stack that we will switch to in order to run the caller's
 * function. 8K is more or less empirically chosen as "enough to run a trivial
 * printf."
 */
static unsigned char emergency_stack[8 * 1024]
    __attribute__((aligned(sizeof(long long))));

void *debug_run_on_emergency_stack(void *(*f)(void *), void *arg) {
    /* Take lock */
    while (!__sync_bool_compare_and_swap(&stack_lock, 0, 1));

    void *ret = (void*)utils_run_on_stack(
        (void*)emergency_stack + sizeof(emergency_stack), f, arg);

    /* Release lock */
    while (!__sync_bool_compare_and_swap(&stack_lock, 1, 0));

    return ret;
}

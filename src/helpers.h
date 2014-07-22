/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef SEL4UTILS_HELPERS_H
#define SEL4UTILS_HELPERS_H
/**
 * Start a thread.
 *
 * @param stack_top address of the stack in the current vspace.
 * Other parameters see sel4utils_start_thread.
 */
int sel4utils_internal_start_thread(sel4utils_thread_t *thread, void *entry_point, void *arg0,
        void *arg1, int resume, void *stack_top);


#endif /* SEL4UTILS_HELPERS_H */

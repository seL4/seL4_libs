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
 * @param local_stack_top address of the stack in the current vspace, or NULL if arguments
 *                        have already been placed on the stack
 * @param dest_stack_top  stack pointer to set in the newly running thread. This is passed in
 *                        instead of being grabbed from thread in case values have been pushed
 *                        onto it. Only one of local_stack_top and dest_stack_top should be set
 * Other parameters see sel4utils_start_thread.
 */
int sel4utils_internal_start_thread(sel4utils_thread_t *thread, void *entry_point, void *arg0,
                                    void *arg1, int resume, void *local_stack_top, void *dest_stack_top);


#endif /* SEL4UTILS_HELPERS_H */

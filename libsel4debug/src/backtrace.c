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
#include <autoconf.h>
#include <sel4/sel4.h>
#include <sel4debug/instrumentation.h>

/* Don't instrument seL4_GetIPCBuffer so we don't recurse. */
seL4_IPCBuffer *seL4_GetIPCBuffer(void) __attribute__((no_instrument_function));

/* We can't just store backtrace information in a single static area because it
 * needs to be tracked per-thread. To do this we assume each thread has a
 * different IPC buffer and write backtrace information into the area of memory
 * directly following the IPC buffer. Note that this assumes that such an area
 * is mapped and not in use for anything else. This is not true in environments
 * like CAmkES, where you will have to tweak the following #define for the base
 * of this region. We also assume that the size of the stack is initialised to
 * 0 for us because seL4 zeroes frames on creation. The stored information looks
 * like this:
 *
 *   |        ↑        |
 *   | backtrace stack |
 *   +-----------------+
 *   |   stack size    |
 *   +-----------------+ BACKTRACE_BASE
 */
#define BACKTRACE_BASE (((void*)seL4_GetIPCBuffer()) + sizeof(seL4_IPCBuffer))

int backtrace(void **buffer, int size) {
    int *bt_stack_sz = (int*)BACKTRACE_BASE;
    void **bt_stack = (void**)(BACKTRACE_BASE + sizeof(int));

    /* Write as many entries as we can, starting from the top of the stack,
     * into the caller's buffer.
     */
    int i;
    for (i = 0; i < size && i < *bt_stack_sz; i++) {
        buffer[i] = bt_stack[*bt_stack_sz - 1 - i];
    }

    return i;
}

#ifdef CONFIG_LIBSEL4DEBUG_FUNCTION_INSTRUMENTATION_BACKTRACE

void __cyg_profile_func_enter(void *func, void *caller) {
    if (seL4_GetIPCBuffer() == NULL) {
        /* The caller doesn't have a valid IPC buffer. Assume it has not been
         * setup yet and just skip logging the current function.
         */
        return;
    }
    int *bt_stack_sz = (int*)BACKTRACE_BASE;
    void **bt_stack = (void**)(BACKTRACE_BASE + sizeof(int));

    /* Push the current function */
    bt_stack[*bt_stack_sz] = func;
    *bt_stack_sz += 1;
}

void __cyg_profile_func_exit(void *func, void *caller) {
    if (seL4_GetIPCBuffer() == NULL) {
        return;
    }
    int *bt_stack_sz = (int*)BACKTRACE_BASE;

    /* Pop the current function */
    *bt_stack_sz -= 1;
}

#endif

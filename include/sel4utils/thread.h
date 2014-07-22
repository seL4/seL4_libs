/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/**
 *
 * Provides basic thread configuration/starting/cleanup functions.
 *
 * Any other operations (start, stop, resume) should use the seL4 API directly on
 * sel4utils_thread_t->tcb.cptr.
 *
 */
#ifndef _SEL4UTILS_THREAD_H
#define _SEL4UTILS_THREAD_H

#include <autoconf.h>

#ifdef CONFIG_LIB_SEL4_VSPACE

#include <sel4/sel4.h>

#include <vka/vka.h>

#include <vspace/vspace.h>

typedef struct sel4utils_thread {
    vka_object_t tcb;
    void *stack_top;
    seL4_CPtr ipc_buffer;
    seL4_Word ipc_buffer_addr;
} sel4utils_thread_t;

/**
 * Configure a thread, allocating any resources required.
 *
 * @param vka initialised vka to allocate objects with
 * @param alloc initialised vspace structure to allocate virtual memory with
 * @param fault_endpoint endpoint to set as the threads fault endpoint. Can be 0.
 * @param priority seL4 priority for the thread to be scheduled with.
 * @param cspace the root of the cspace to start the thread in
 * @param cspace_root_data data for cspace access
 * @param res an uninitialised sel4utils_thread_t data structure that will be initialised
 *            after this operation.
 *
 * @return 0 on success, -1 on failure. Use CONFIG_DEBUG to see error messages.
 */
int sel4utils_configure_thread(vka_t *vka, vspace_t *alloc, seL4_CPtr fault_endpoint,
        uint8_t priority, seL4_CNode cspace, seL4_CapData_t cspace_root_data,
        sel4utils_thread_t *res);

/**
 * Start a thread, allocating any resources required.
 * The third argument to the thread (in r2 for arm, on stack for ia32) will be the
 * address of the ipc buffer.
 *
 * @param thread      thread data structure that has been initialised with sel4utils_configure_thread
 * @param entry_point the address that the thread will start at
 *                     
 *                    NOTE: In order for the on-stack argument passing to work for ia32, 
 *                    entry points must be functions. 
 *
 *                    ie. jumping to this start symbol will work:
 *
 *                    void _start(int argc, char **argv) {
 *                        int ret = main(argc, argv);
 *                        exit(ret);
 *                    }
 * 
 *
 *                    However, jumping to a start symbol like this:
 *
 *                    _start:
 *                         call main
 *
 *                    will NOT work, as call pushes an extra value (the return value)
 *                    onto the stack. If you really require an assembler stub, it should
 *                    decrement the stack value to account for this.
 *
 *                    ie.
 *
 *                    _start:
 *                         popl %eax
 *                         call main
 *
 *                    This does not apply for arm, as arguments are passed in registers.
 *
 *
 * @param arg0        a pointer to the arguments for this thread. User decides the protocol.
 * @param arg1        another pointer. User decides the protocol. Note that there are two args here
 *                    to easily support C standard: int main(int argc, char **argv).
 * @param resume      1 to start the thread immediately, 0 otherwise.
 *
 * @return 0 on success, -1 on failure.
 */
int sel4utils_start_thread(sel4utils_thread_t *thread, void *entry_point, void *arg0, void *arg1,
        int resume);

/**
 * Release any resources used by this thread. The thread data structure will not be usable
 * until sel4utils_thread_configure is called again.
 *
 * @param vka the vka interface that this thread was initialised with
 * @param alloc the allocation interface that this thread was initialised with
 * @param thread the thread structure that was returned when the thread started
 */
void sel4utils_clean_up_thread(vka_t *vka, vspace_t *alloc, sel4utils_thread_t *thread);

/**
 * Start a fault handling thread that will print the name of the thread that faulted
 * as well as debugging information.
 *
 * @param fault_endpoint the fault_endpoint to wait on
 * @param vka allocator
 * @param vspace vspace (this library must be mapped into that vspace).
 * @param prio the priority to run the thread at (recommend highest possible)
 * @param cspace the cspace that the fault_endpoint is in
 * @param data the cspace_data for that cspace (with correct guard)
 * @param name the name of the thread to print if it faults
 * @param thread the thread data structure to populate
 *
 * @return 0 on success.
 */
int sel4utils_start_fault_handler(seL4_CPtr fault_endpoint, vka_t *vka, vspace_t *vspace, 
        uint8_t prio, seL4_CPtr cspace, seL4_CapData_t data, char *name, sel4utils_thread_t *res);
 

/**
 * Pretty print a fault messge.
 *
 * @param tag the message info tag delivered by the fault.
 * @param name thread name 
 */
void sel4utils_print_fault_message(seL4_MessageInfo_t tag, char *name);

static inline seL4_TCB
sel4utils_get_tcb(sel4utils_thread_t *thread)
{
    return thread->tcb.cptr;
}

static inline int
sel4utils_suspend_thread(sel4utils_thread_t *thread)
{
    return seL4_TCB_Suspend(thread->tcb.cptr);
}

#endif /* CONFIG_LIB_SEL4_VSPACE */
#endif /* _SEL4UTILS_THREAD_H */

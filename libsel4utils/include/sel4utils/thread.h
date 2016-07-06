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
#include <simple/simple.h>
#include <stdbool.h>
#include <vka/vka.h>

#include <vspace/vspace.h>

typedef struct sel4utils_thread {
    vka_object_t tcb;
    void *stack_top;
    size_t stack_size;
    seL4_CPtr ipc_buffer;
    seL4_Word ipc_buffer_addr;
    vka_object_t sched_context;
    bool own_sc;
} sel4utils_thread_t;

typedef struct sel4utils_thread_config {
    /* fault_endpoint endpoint to set as the threads fault endpoint. Can be seL4_CapNull. */
    seL4_CPtr fault_endpoint;
    /* temporal fault_endpoint endpoint to set as the threads temporal fault endpoint. Can be seL4_CapNull. */
    seL4_CPtr temporal_fault_endpoint;
    /* seL4 priority for the thread to be scheduled with. */
    uint8_t priority;
    /* max priority for that the thread can create and set other threads to */
    uint8_t mcp;
    /* seL4 criticality for the thread to be scheduled with. */
    uint8_t criticality;
    /* max criticality that the thread can create and set other threads to */
    uint8_t mcc;
    /* root of the cspace to start the thread in */
    seL4_CNode cspace;
    /* data for cspace access */
    seL4_CapData_t cspace_root_data;
    /* true if configure should allocate a sched context -
     * if this is set then you must also provide a simple interface to the
     * configure call that can provide the seL4_SchedControl cap. */
    bool create_sc;
    /* true if that sched context should use custom params */
    bool custom_sched_params;
    /* the custom params */
    seL4_Time custom_budget;
    seL4_Time custom_period;
    /* otherwise provide a custom scheduling context cap */
    seL4_CPtr sched_context;
    /* use a custom stack size? */
    bool custom_stack_size;
    /* custom stack size in 4k pages for this thread */
    seL4_Word stack_size;
} sel4utils_thread_config_t;

typedef struct sel4utils_checkpoint {
    void *stack;
    seL4_UserContext regs;
    sel4utils_thread_t *thread;
} sel4utils_checkpoint_t;

/**
 * Configure a thread, allocating any resources required.
 *
 * @param simple a simple that can provide the sched_ctrl cap. If create_sc is not set to true
 *               in the thread config this is not required.
 * @param vka initialised vka to allocate objects with
 * @param parent vspace structure of the thread calling this function, used for temporary mappings
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
int sel4utils_configure_thread(simple_t *simple, vka_t *vka, vspace_t *parent, vspace_t *alloc, 
                               seL4_CPtr fault_endpoint,
                               uint8_t priority, seL4_CNode cspace, seL4_CapData_t cspace_root_data,
                               sel4utils_thread_t *res);


/**
 * As per sel4utils_configure_thread, but using a config struct.
 */
int sel4utils_configure_thread_config(simple_t *simple, vka_t *vka, vspace_t *parent, vspace_t *alloc,
                                      sel4utils_thread_config_t config, sel4utils_thread_t *res);

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
 * Checkpoint a thread at its current state, storing its current register set and stack.
 *
 * Note that the heap state is not saved, so threads intending to use this functionality
 * should not mutate the heap or other state beyond the checkpoint, unless extra functionality
 * is included to roll these back.
 *
 * This should not be called on a currently running thread.
 *
 * @param thread     the thread to checkpoint
 * @param checkpoint pointer to uninitialised checkpoint struct
 * @param suspend    true if the thread should be suspended 
 * 
 * @return 0 on success.
 */
int sel4utils_checkpoint_thread(sel4utils_thread_t *thread, sel4utils_checkpoint_t *checkpoint, bool suspend);

/**
 * Rollback a thread to a previous checkpoint, restoring its register set and stack. 
 *
 * This is not atomic and callers should make sure the target thread is stopped or that the 
 * caller is higher priority such that the target is not switched to by the kernel mid-restore. 
 *
 * @param checkpoint the previously saved checkpoint to restore.
 * @param free       true if this checkpoint should free all memory allocated, i.e if the checkpoint
 *                   will not be used again.
 * @param resume     true if the thread should be resumed immediately. 
 *
 * @return 0 on success.
 */
int sel4utils_checkpoint_restore(sel4utils_checkpoint_t *checkpoint, bool free, bool resume);

/**
 * Clean up a previously allocated checkpoint.
 */
void sel4utils_free_checkpoint(sel4utils_checkpoint_t *checkpoint);

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
int sel4utils_start_fault_handler(seL4_CPtr fault_endpoint, simple_t *simple, vka_t *vka, vspace_t *vspace,
                                  uint8_t prio, seL4_CPtr cspace, seL4_CapData_t data, char *name, sel4utils_thread_t *res);


/**
 * Pretty print a fault message.
 *
 * @param tag the message info tag delivered by the fault.
 * @param name thread name
 */
void sel4utils_print_fault_message(seL4_MessageInfo_t tag, const char *name);

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

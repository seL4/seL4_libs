/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

/**
 *
 * Provides basic thread configuration/starting/cleanup functions.
 *
 * Any other operations (start, stop, resume) should use the seL4 API directly on
 * sel4utils_thread_t->tcb.cptr.
 *
 */
#pragma once

#include <autoconf.h>

#include <sel4/sel4.h>
#include <stdbool.h>
#include <vka/vka.h>

#include <vspace/vspace.h>
#include <sel4utils/thread_config.h>

#ifdef CONFIG_DEBUG_BUILD
#define NAME_THREAD(_tcbcap, _name)   seL4_DebugNameThread(_tcbcap, _name);
#else
#define NAME_THREAD(_tcbcap, _name)
#endif

typedef struct sel4utils_thread {
    vka_object_t tcb;
    vka_object_t sched_context;
    void *stack_top;
    void *initial_stack_pointer;
    size_t stack_size;
    seL4_CPtr ipc_buffer;
    seL4_Word ipc_buffer_addr;
    bool own_sc;
    bool own_reply;
    vka_object_t reply;
} sel4utils_thread_t;

typedef struct sel4utils_checkpoint {
    void *stack;
    seL4_UserContext regs;
    sel4utils_thread_t *thread;
} sel4utils_checkpoint_t;

typedef void (*sel4utils_thread_entry_fn)(void *arg0, void *arg1, void *ipc_buf);

/**
 * Configure a thread, allocating any resources required. The thread will start at priority 0.
 *
 * If CONFIG_RT is enabled, the thread will not have a scheduling context, so it will not be able to run.
 *
 * @param vka initialised vka to allocate objects with
 * @param parent vspace structure of the thread calling this function, used for temporary mappings
 * @param alloc initialised vspace structure to allocate virtual memory with
 * @param fault_endpoint endpoint to set as the threads fault endpoint. Can be 0.
 * @param cspace the root of the cspace to start the thread in
 * @param cspace_root_data data for cspace access
 * @param res an uninitialised sel4utils_thread_t data structure that will be initialised
 *            after this operation.
 *
 * @return 0 on success, -1 on failure. Use CONFIG_DEBUG to see error messages.
 */
int sel4utils_configure_thread(vka_t *vka, vspace_t *parent, vspace_t *alloc, seL4_CPtr fault_endpoint,
                               seL4_CNode cspace, seL4_Word cspace_root_data,
                               sel4utils_thread_t *res);

/**
 * As per sel4utils_configure_thread, but using a config struct.
 */
int sel4utils_configure_thread_config(vka_t *vka, vspace_t *parent, vspace_t *alloc,
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
int sel4utils_start_thread(sel4utils_thread_t *thread, sel4utils_thread_entry_fn entry_point,
                           void *arg0, void *arg1, int resume);

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
 * as well as debugging information. The thread will start at priority 0.
 *
 * If CONFIG_RT it be passive (not have a scheulding context) and will run on the SC of the faulter.
 *
 * @param fault_endpoint the fault_endpoint to wait on
 * @param vka allocator
 * @param vspace vspace (this library must be mapped into that vspace).
 * @param cspace the cspace that the fault_endpoint is in
 * @param data the cspace_data for that cspace (with correct guard)
 * @param name the name of the thread to print if it faults
 * @param thread the thread data structure to populate
 *
 * @return 0 on success.
 */
int sel4utils_start_fault_handler(seL4_CPtr fault_endpoint, vka_t *vka, vspace_t *vspace,
                                  seL4_CPtr cspace, seL4_Word data, char *name, sel4utils_thread_t *res);

/**
 * Pretty print a fault message.
 *
 * @param tag the message info tag delivered by the fault.
 * @param name thread name
 */
void sel4utils_print_fault_message(seL4_MessageInfo_t tag, const char *name);

/* Set the affinity of a thread, which will cause migration if the thread
 * is running on a different core.
 *
 * On master, only use sched_params_set_core to set the core then call this function.
 *
 * If CONFIG_KERNEL_RT is set, the sched params must be fully populated or the scheduling
 * context will be empty when it changes core as scheduling parameters of scheduling
 * contexts are not maintained across migrations.
 */
int sel4utils_set_sched_affinity(sel4utils_thread_t *thread, sched_params_t params);

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


/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef __SEL4UTILS_SCHED_H
#define __SEL4UTILS_SCHED_H

#include <sel4/sel4.h>
#include <sel4utils/thread.h>
#include <sel4platsupport/timer.h>
#include <stdint.h>
#include <vka/vka.h>

typedef struct sched sched_t;

struct edf_sched_add_tcb_args {
    /* Cap to the tcb, which may already be running. This thread should be the same
       priority as all other threads added to this EDF scheduler. The EDF scheduler
       itself should run at a higher priority than the threads it is scheduling. */
    seL4_CPtr tcb;
    /* scheduling parameters for this thread. Does not have to be the same parameters
       that the scheduling context for this thread uses, but should not exceed them
       or EDF scheduling is not guaranteed. */
    time_t budget;
    time_t period;
    /* empty slot to save a minted endpoint cap in, threads should Call on this
       endpoint to signal job completion and wait for next job release. */
    cspacepath_t slot;
};

struct cfs_sched_add_tcb_args {
    /* empty slot to save a minted endpoint cap in, threads should Call on this
       endpoint to signal job completion and wait for next job release. Only
       used in coop cfs implementation. */
    cspacepath_t slot;
};

/* generic scheduling interface */
struct sched {
    void *(*add_tcb)(sched_t *sched, seL4_CPtr sched_context, void *args);
    void (*remove_tcb)(sched_t *sched, void *tcb);
    void (*reset)(sched_t *sched);
    int (*run_scheduler)(sched_t *sched, bool (*finished)(void *cookie), void *cookie, void *args);
    void (*destroy_scheduler)(sched_t *sched);
    void *data;
};

/*
 * Create an EDF scheduler. Multiple EDF schedulers can co-exist, however they should run on distinct
 * priorities.
 *
 * @param clock_timer   initialised timer interface that can be used for a 64-bit, upcounting timestamp.
 * @param timeout_timer initialised timer interface that supports 32 bit relative timeouts
 * @param vka           vka interface to allocate objects and cslots of the scheduler
 * @param tcb           cap to the tcb of the thread that the scheduler will run in.
 * @param notification  notification that irqs from timeout_timer are delivered on.
 *
 * @return an initialised EDF scheduling interface.
 *         NULL on error.
 */
sched_t *sched_new_edf(seL4_timer_t *clock_timer, seL4_timer_t *timeout_timer, vka_t *vka,
                       seL4_CPtr tcb, seL4_CPtr notification);

/*
 * Create a Linux-like CFS scheduler. Multiple CFS schedulers can co-exist, however they should run on distinct
 * priorities.
 *
 * The preemptive CFS scheduler expects scheduling contexts to be configured with the timeslice
 * expected for the thread. Threads will be preempted by the kernel when they run out of time.
 * The scheduler thread should run at the same prio as the threads it is scheduling, and the
 * caller should make sure the scheduler is at the head of the scheduling queue.
 *
 * @param vka vka interface to allocate objects and cslots of the scheduler
 */
sched_t *sched_new_preemptive_cfs(void);

/*
 * Create a Linux-like CFS scheduler. Multiple CFS schedulers can co-exist, however they should run on distinct
 * priorities.
 *
 * This cooperative CFS scheduler expects threads to call on an endpoint to
 * manually yield. Threads will be converted to passive, such that the inital sc
 * threads start with will be unbound.
 *
 * @param vka           vka interface to allocate objects and cslots of the scheduler
 * @param sched_context the scheduling context the scheduler will run on (caller of run_scheduler)
 */
sched_t *sched_new_cooperative_cfs(vka_t *vka, seL4_CPtr sched_context);

/*
 * Add a TCB to a scheduler. The scheduler must be initialised and may or may not be running.
 *
 * @param sched         initialised scheduler.
 * @param sched_context capability to this thread's scheduling context.
 * @param params        scheduler specific parameters for this function
 *
 * @return NULL on failure, otherwise a reference to the TCB that can be used to remove it from the
 *         the scheduler
 */
static inline void *
sched_add_tcb(sched_t *sched, seL4_CPtr sched_context, void *params)
{
    assert(sched != NULL);
    return sched->add_tcb(sched, sched_context, params);
}

/*
 * Remove a TCB from the scheduler if it is in the scheduler.
 *
 * @param sched initialised scheduler.
 * @param tcb reference to tcb returned by sched_add_tcb.
 */
static inline void
sched_remove_tcb(sched_t *sched, void *tcb)
{
    assert(sched != NULL);
    sched->remove_tcb(sched, tcb);
}


/*
 * Remove all threads from the scheduler and reset to the same state as if sched_new_* had
 * just been called
 */
static inline void
sched_reset(sched_t *sched)
{
    assert(sched != NULL);
    sched->reset(sched);
}


/**
 * Use the scheduler to run threads.
 *
 * Threads should have their own scheduling contexts, and already be runnable.
 *
 * If finished() returns false the scheduler can be resumed again by calling this function.
 *
 * @param sched     initialised, not running, scheduler.
 * @param finished function to call which returns true when the scheduler should
 *                 stop scheduling (can just return false for infinite scheduling)
 * @param cookie   argument to pass to the finished function, can be NULL.
 * @param args     custom argument for this specific scheduler
 * @return 0 on success, -1 on failure.
 */
static inline int
sched_run(sched_t *sched, bool (*finished)(void *cookie), void *cookie, void *args)
{
    assert(sched != NULL);
    assert(finished != NULL);

    return sched->run_scheduler(sched, finished, cookie, args);
}

/*
 * Destroy an initialised scheduler, freeing any resources.
 */
static inline void
sched_destroy_scheduler(sched_t *sched)
{
    assert(sched != NULL);
    sched->destroy_scheduler(sched);
}

#endif /* __SEL4UTILS_SCHED_H */

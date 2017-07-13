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

#define MAX_RES_VAL 2
#define NUM_THREADS 4
#define WAIT_BOUND 2

/* A notification in seL4 is a single data word.
 * Here we simplify things to a single bit */
typedef notification_t {
    bit data = 1;
};

/* Condition variable */
typedef condition_var_t {
    notification_t wait_nb;
    int waiters = 0;
    bit broadcasting = 0;
};

/* Monitor */
typedef monitor_t {
    notification_t lock;
    condition_var_t consumer_cv;
    condition_var_t producer_cv;
};

/* The shared resource.
 * Producer increments when resource < MAX_RES_VAL
 * Consumer decrements when resource > 0.
 */
byte resource = 0;

/* Global monitor */
monitor_t the_monitor;

/* Some thread state used to check correctness */
byte thread_state[4];
/*  0 - Default
    1 - Has the monitor lock (i.e. is in the critical region)
    2 - Waiting on consumer_cv
    3 - Waiting on producer_cv
    4 - Sees a broadcast
    5 - thread terminated
*/

/* We record the number of times a thread has waited in order to terminate
   This is necessary to avoid infinite loops for which we can prove nothing */
byte thread_waits[4];

/* seL4_Wait blocks on the notification.
 * When the data word is 1 it resets to 0 and returns. */
inline seL4_Wait(notif)
{
    atomic {
        (notif.data == 1) -> notif.data = 0;
    }
}

/* seL4_Signal updates the notification, it is non-blocking.
 * It is a bitwise OR of the data word */
inline seL4_Signal(notif)
{
    notif.data = 1;
}

/* Wait on a condition variable. The calling thread must own the monitor lock */
inline cv_wait(monitor, cv, cv_id, procnum)
{
    /* Increment the waiters count */
    cv.waiters++;

    /* Release the monitor lock */
    printf("%d signals monitor.lock and waits\n", procnum);
    atomic {
        seL4_Signal(monitor.lock);
        printf("thread_state[%d] = %d\n", procnum-1, cv_id);
        thread_state[procnum-1] = cv_id;
    }

    /* Sleep until notified */
    printf("%d waits on cv.nb\n", procnum);
    atomic {
        seL4_Wait(cv.wait_nb);
        thread_state[procnum-1] = 0;
        printf("thread_state[%d] = %d\n", procnum-1, 0);
    }

    int oldval;
    atomic {
        oldval = cv.waiters;
        cv.waiters--;
    }

    /* Signal other waiting threads if we're broadcasting */
    if
    :: (oldval > 1 && cv.broadcasting == 1) -> seL4_Signal(cv.wait_nb);
    :: (oldval <= 1 && cv.broadcasting == 1) -> cv.broadcasting = 0;
    :: else -> skip;
    fi

    /* Reaquire the lock */
    printf("%d waits on monitor.lock\n", procnum);
    atomic {
        monitor_acquire(monitor);
        thread_state[procnum-1] = 1;
        printf("%d acquires monitor lock\n", procnum);
    }

    printf("%d wakes up\n", procnum);
}

inline cv_signal(monitor, cv, procnum)
{
    /* Decrement the waiters count */
    printf("%d signals cv\n", procnum);
    if
    :: (cv.waiters > 0) ->
        /* Wake up a thread */
        printf("Signal wait_nb\n");
        seL4_Signal(cv.wait_nb);
    :: else ->  skip;
    fi

}

inline cv_broadcast(monitor, cv, cv_id, procnum)
{
    printf("%d broadcasts cv\n", procnum);
    if
    :: (cv.waiters > 0) ->
        if
        :: (thread_state[0] == cv_id && thread_waits[0] != WAIT_BOUND) -> thread_state[0] = 4;
        :: skip;
        fi

        if
        :: (thread_state[1] == cv_id && thread_waits[1] != WAIT_BOUND) -> thread_state[1] = 4;
        :: skip;
        fi

        if
        :: (thread_state[2] == cv_id && thread_waits[2] != WAIT_BOUND) -> thread_state[2] = 4;
        :: skip;
        fi

        if
        :: (thread_state[3] == cv_id && thread_waits[3] != WAIT_BOUND) -> thread_state[3] = 4;
        :: skip;
        fi

        /* Wake up a thread */
        printf("Signal wait_nb (broadcast, waiters = %d)\n", cv.waiters);
        cv.broadcasting = 1;
        seL4_Signal(cv.wait_nb);
    :: else ->  skip;
    fi
}

inline monitor_acquire(monitor)
{
    seL4_Wait(monitor.lock);
}

inline monitor_release(monitor)
{
    seL4_Signal(monitor.lock);
}

proctype producer(byte procnum)
{
end: do
    :: true;
progress_prod:;
        atomic {
            monitor_acquire(the_monitor);
            thread_state[procnum-1] = 1;
        }
        printf("producer acquires m\n");

        /* Producer waits for resource != MAX_RES_VAL */
        do
        :: (resource == MAX_RES_VAL) ->
            cv_wait(the_monitor, the_monitor.producer_cv, 3, procnum);
            thread_waits[procnum-1]++;
            if
            :: (thread_waits[procnum-1] >= WAIT_BOUND) -> break;
            :: else -> skip;
            fi
        :: else -> thread_waits[procnum-1]++; break;
        od

        if
        :: (thread_waits[procnum-1] >= WAIT_BOUND) -> monitor_release(the_monitor); break;
        :: else -> skip;
        fi

        /* Critical section */
        assert(resource != MAX_RES_VAL);
        resource++;
        printf("resource = %d\n", resource);

        /* cv_signal(the_monitor, the_monitor.producer_cv, procnum);
        cv_signal(the_monitor, the_monitor.consumer_cv, procnum);
        printf("producer signals cv\n"); */

        if
        :: (resource == MAX_RES_VAL) ->
            printf("producer broadcasts\n");
            cv_broadcast(the_monitor, the_monitor.consumer_cv, 2, procnum);
        :: else ->
            skip;
            /* printf("consumer signals\n");
            cv_signal(the_monitor, the_monitor.consumer_cv, procnum); */
        fi

        atomic {
            monitor_release(the_monitor);
            thread_state[procnum-1] = 0;
        }
        printf("producer releases m\n");
    od
    printf("%d dies with thread_state = %d\n", procnum, thread_state[procnum-1]);
    thread_state[procnum-1] = 5;
    cv_signal(the_monitor, the_monitor.consumer_cv, procnum);
}

proctype consumer(byte procnum)
{
end: do
    :: true;
progress_cons:;
        atomic {
            monitor_acquire(the_monitor);
            thread_state[procnum-1] = 1;
        }
        printf("consumer acquires m\n");

        /* Consumer waits for resource != 0 */
        do
        :: (resource == 0) ->
            cv_wait(the_monitor, the_monitor.consumer_cv, 2, procnum);
            thread_waits[procnum-1]++;
            if
            :: (thread_waits[procnum-1] >= WAIT_BOUND) -> break;
            :: else -> skip;
            fi
        :: else -> thread_waits[procnum-1]++; break;
        od

        if
        :: (thread_waits[procnum-1] >= WAIT_BOUND) -> monitor_release(the_monitor); break;
        :: else -> skip;
        fi

        /* Critical section */
        assert(resource != 0);
        resource--;
        printf("resource = %d\n", resource);

        if
        :: (resource == 0) ->
            printf("consumer broadcasts\n");
            printf("the_monitor.lock.data = %d\n", the_monitor.lock.data);
            cv_broadcast(the_monitor, the_monitor.producer_cv, 3, procnum);
        :: else ->
            skip;
            /* printf("consumer signals\n");
            cv_signal(the_monitor, the_monitor.consumer_cv, procnum); */
        fi

        atomic {
            monitor_release(the_monitor);
            thread_state[procnum-1] = 0;
            printf("consumer releases m\n");
        }
    od
    printf("%d dies with thread_state = %d\n", procnum, thread_state[procnum-1]);
    thread_state[procnum-1] = 5;
    cv_signal(the_monitor, the_monitor.producer_cv, procnum);
}

init {
    thread_state[0] = 0;
    thread_state[1] = 0;
    thread_state[2] = 0;
    thread_state[3] = 0;

    thread_waits[0] = 0;
    thread_waits[1] = 0;
    thread_waits[2] = 0;
    thread_waits[3] = 0;

    atomic {
        run producer(1);
        run producer(2);
        run consumer(3);
        run consumer(4);
    }
}

/* No more than one state may be in the critical region */
/* ltl mutual_exclusion { [](
       ((thread_state[0] == 1) -> !(thread_state[1] == 1 || thread_state[2] == 1 || thread_state[3] == 1))
    && ((thread_state[1] == 1) -> !(thread_state[0] == 1 || thread_state[2] == 1 || thread_state[3] == 1))
    && ((thread_state[2] == 1) -> !(thread_state[0] == 1 || thread_state[1] == 1 || thread_state[3] == 1))
    && ((thread_state[3] == 1) -> !(thread_state[0] == 1 || thread_state[1] == 1 || thread_state[2] == 1)))} */

/* If a thread is waiting during a broadcast, eventually it wakes up */
ltl broadcast0 { []( (thread_state[0] == 4) -> (<> (thread_state[0] == 1)) ) }
ltl broadcast1 { []( (thread_state[1] == 4) -> (<> (thread_state[1] == 1)) ) }
ltl broadcast2 { []( (thread_state[2] == 4) -> (<> (thread_state[2] == 1)) ) }
ltl broadcast3 { []( (thread_state[3] == 4) -> (<> (thread_state[3] == 1)) ) }

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

#define send_enabled 1

/* An async endpoint is implemented by a channel of length 1 that
 * drops messages when full. Dropping of messages is done by passing
 * the -m switch to spin */
chan endpoint = [1] of {bit};

/* binary semaphore starts with value of 1 */
int sem_value = 1;

inline bin_sem_wait() {
    int oldval;

    /* Atomic decrement */
    atomic {
        oldval = sem_value;
        sem_value--;
    }

    /* Conditional wait */
    if
    :: (oldval <= 0) -> endpoint ? 1;
    :: (oldval > 0) -> skip
    fi
}

inline bin_sem_signal()
{
    int new_val;

    /* Atomic increment. */
    atomic {
        sem_value++;
        new_val = sem_value;
    }

    /* Conditional wake. */
    if
    :: (new_val <= 0) ->
        send: endpoint ! 1;
    :: (new_val > 0) ->
        skip;
    fi
}

active [4] proctype wait_thread() {
    do
    :: true ->
        /* lock */
        bin_sem_wait();

        /* critical section */
        crit:;

        /* unlock */
        bin_sem_signal();
    od
}

/* Verify that the -m option was used. If it wasn't we might
 * block on a send, this will make blocking a failure case */
ltl cansend { []((wait_thread[0]@crit)->enabled(0)) }

/* Verify mutual exclusion */
ltl mutex { []( (wait_thread[0]@crit) -> !(wait_thread[1]@crit) ) }

/* Formulate liveness as a safety property by stating we will
 * always be able to get the critical section again */
ltl liveness { []<>(wait_thread[0]@crit || wait_thread[1]@crit || wait_thread[2]@crit || wait_thread[3]@crit) }

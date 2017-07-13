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

#define MAX_SEM_VAL 1
#define MAX_RES_VAL 2
#define MAX_PROC 2

/* A notification in seL4 is a single data word.
 * Here we simplify things to a single bit */
typedef notification_t {
    bit data = 0;
};

/* Condition variable */
typedef condition_var_t {
    notification_t nb;
};

/* Monitor */
typedef semaphore_t {
    notification_t lock;
    int count = MAX_SEM_VAL;
};

/* Users in critical section. */
byte users = 0;

semaphore_t sem;

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

inline do_wait(procnum)
{
    int oldval;

    /* Atomic decrement. */
    atomic {
        oldval = sem.count;
        sem.count--;
    }

    /* Conditional wait. */
    if
    :: (oldval <= 0) -> seL4_Wait(sem.lock);
    :: (oldval > 0) -> skip;
    fi
}

inline do_signal(procnum)
{
    int new_val;

    /* Atomic decrement. */
    atomic {
        sem.count++;
        new_val = sem.count;
    }

    /* Conditional wake. */
    if
    :: (new_val <= 0) -> seL4_Signal(sem.lock);
    :: (new_val > 0) -> skip;
    fi
}

proctype wait_cpu(byte procnum)
{
    do
    :: true;
progress_wait:;
        /* Lock. */
        printf("%d getting the lock.\n", procnum);
        do_wait(procnum);
        printf("%d got the lock.\n", procnum);

        /* Use a common resource. */
        users++;
        assert(users <= MAX_SEM_VAL);
        users--;

        /* Release. */
        printf("%d releasing the lock\n", procnum);
        do_signal(procnum);
        printf("%d released the lock.\n", procnum);
    od
}

init {
    atomic {
        run wait_cpu(1);
        run wait_cpu(2);
        run wait_cpu(3);
    }
}

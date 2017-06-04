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

#ifndef _LIBSEL4DEBUG_FAULTS_H_
#define _LIBSEL4DEBUG_FAULTS_H_

/* A basic fault handler for debugging purposes. Note that you are expected to
 * call this function from the thread that you wish to act as a fault handler
 * as it doesn't return on success. Returns non-zero on failure.
 *
 *  faultep - The endpoint to listen on for fault messages.
 *  senders - A function for translating endpoint badge to thread name. This is
 *      helpful if you want to handle faults from multiple threads via badged
 *      endpoints, but are also in a position to provide this handler with more
 *      information (than just the badge) about which thread faulted. If this
 *      parameter is NULL it will be ignored.
 *
 * TODO: This function has not been tested. I wrote it with the aim of
 * debugging some CAmkES code, but there is an open bug (VER-348) that prevents
 * setting fault handlers currently.
 */
int debug_fault_handler(seL4_CPtr faultep,
                        const char * (*senders)(seL4_Word badge));

#endif

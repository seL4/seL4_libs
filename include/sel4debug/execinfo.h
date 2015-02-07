/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _LIBSEL4DEBUG_EXECINFO_H_
#define _LIBSEL4DEBUG_EXECINFO_H_

/* Retrieve a backtrace for the current thread as a list of pointers and place
 * the information in buffer. The argument size should be the number of void*
 * elements that will fit into buffer. The return value is the actual number of
 * entries that are obtained, and is at most size.
 *
 * This requires the setting LIBSEL4DEBUG_FUNCTION_INSTRUMENTATION_BACKTRACE.
 */
int backtrace(void **buffer, int size) __attribute__((no_instrument_function));

#endif

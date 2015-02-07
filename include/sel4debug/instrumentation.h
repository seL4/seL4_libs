/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _LIBSEL4DEBUG_INSTRUMENTATION_H_
#define _LIBSEL4DEBUG_INSTRUMENTATION_H_

/* Instrumentation functions that are optionally provided by this library.
 * These need to be explicitly tagged as no_instrument_function or GCC will
 * instrument the instrumentation functions themselves causing infinite
 * recursion. Why it would ever be desirable to instrument your instrumentation
 * is left as an exercise to the reader.
 */

void __cyg_profile_func_enter(void *func, void *caller)
__attribute__((no_instrument_function));

void __cyg_profile_func_exit(void *func, void *caller)
__attribute__((no_instrument_function));

#endif

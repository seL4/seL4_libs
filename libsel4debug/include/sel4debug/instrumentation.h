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

#pragma once

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


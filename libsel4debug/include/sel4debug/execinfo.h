/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

/* Retrieve a backtrace for the current thread as a list of pointers and place
 * the information in buffer. The argument size should be the number of void*
 * elements that will fit into buffer. The return value is the actual number of
 * entries that are obtained, and is at most size.
 *
 * This requires the setting LIBSEL4DEBUG_FUNCTION_INSTRUMENTATION_BACKTRACE.
 */
int backtrace(void **buffer, int size) __attribute__((no_instrument_function));


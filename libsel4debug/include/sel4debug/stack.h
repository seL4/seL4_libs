/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

/* Run the given function on a static stack as a last-ditch debugging measure.
 * Note that this function *does* switch back to the originating stack and
 * return leaving you in a sane state. This function is *not* to be considered
 * thread safe.
 */
void *debug_run_on_emergency_stack(void *(*f)(void *), void *arg);


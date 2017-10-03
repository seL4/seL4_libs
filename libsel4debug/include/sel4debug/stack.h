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

/* Run the given function on a static stack as a last-ditch debugging measure.
 * Note that this function *does* switch back to the originating stack and
 * return leaving you in a sane state. This function is *not* to be considered
 * thread safe.
 */
void *debug_run_on_emergency_stack(void *(*f)(void *), void *arg);


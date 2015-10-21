/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _LIBSEL4DEBUG_STACK_H_
#define _LIBSEL4DEBUG_STACK_H_

/* Run the given function on a static stack as a last-ditch debugging measure.
 * Note that this function *does* switch back to the originating stack and
 * return leaving you in a sane state. This function is *not* to be considered
 * thread safe.
 */
void *debug_run_on_emergency_stack(void *(*f)(void *), void *arg);

#endif

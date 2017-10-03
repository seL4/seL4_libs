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

/* In the case of a fault or other unexpected error, your fault handler or the
 * kernel often does not have enough information to report a helpful message to
 * the user about who it was who caused the fault. The two functions below
 * allow you to save a pointer to a user-friendly identity of the current
 * thread. This can then be retrieved by a fault handler or error routine later
 * in order to print a helpful debugging message.
 */

#pragma once

void debug_set_id(const char *s);
void debug_set_id_fn(const char * (*fn)(void));


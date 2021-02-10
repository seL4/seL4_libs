/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
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


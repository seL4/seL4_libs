/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sel4debug/identity.h>
#include "identity-internal.h"
#include <stdlib.h>

static const char *id_str;
static const char *(*id_fn)(void);

void debug_set_id(const char *s)
{
    id_str = s;
    id_fn = NULL;
}

void debug_set_id_fn(const char * (*fn)(void))
{
    id_fn = fn;
    id_str = NULL;
}

/* Return the identity of the current thread using whatever information the
 * user previously told us. Note, this will return NULL if the user has not
 * initialised either source of thread ID.
 */
const char *debug_get_id(void)
{
    if (id_fn != NULL) {
        return id_fn();
    } else {
        return id_str;
    }
}

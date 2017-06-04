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

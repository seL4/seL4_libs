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

#include <vka/vka.h>

/* A shim allocator for debugging problems with applications (callers of an
 * allocator) or an allocator itself. You shouldn't really need to know much
 * about this allocator to call and use it. Note that when it encounters a
 * perceived problem it will call abort() though.
 *
 * vka - A structure to populate with this allocator's implementation
 *   functions.
 * tracee - A subordinate allocator that will do the actual work. The debug
 *   allocator tracks its (and your) actions and reports problems it finds.
 *
 * Returns 0 on success or -1 on failure to create internal bookkeeping.
 */
int vka_init_debugvka(vka_t *vka, vka_t *tracee);


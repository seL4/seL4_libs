/*
 * Copyright 2018, Data61
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

/* This file provides convenient wrappers around seL4 IPC buffer manipulation */
#include <autoconf.h>
#include <sel4vka/gen_config.h>
#include <vka/cspacepath_t.h>
#include <vka/object.h>

static inline void vka_set_cap_receive_path(const cspacepath_t *src)
{
    seL4_SetCapReceivePath(
        /* _service */      src->root,
        /* index */         src->capPtr,
        /* depth */         src->capDepth
    );
}

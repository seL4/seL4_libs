/*
 * Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
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

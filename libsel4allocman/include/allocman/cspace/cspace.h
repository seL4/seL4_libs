/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sel4/types.h>
#include <allocman/properties.h>
#include <vka/cspacepath_t.h>

struct allocman;

typedef struct cspace_interface {
    int (*alloc)(struct allocman *alloc, void *cookie, cspacepath_t *path);
    void (*free)(struct allocman *alloc, void *cookie, const cspacepath_t *path);
    cspacepath_t (*make_path)(void *cookie, seL4_CPtr slot);
    struct allocman_properties properties;
    void *cspace;
} cspace_interface_t;


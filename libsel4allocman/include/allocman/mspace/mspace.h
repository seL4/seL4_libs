/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdlib.h>
#include <allocman/properties.h>

struct allocman;

struct mspace_interface {
    void *(*alloc)(struct allocman *alloc, void *cookie, size_t bytes, int *error);
    void (*free)(struct allocman *alloc, void *cookie, void *ptr, size_t bytes);
    struct allocman_properties properties;
    void *mspace;
};


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

/* For every type of resource allocator we have some properties we would
   like to know about it to make allocation decisions. These are gathered
   here in this struct */

struct allocman_properties {
    int alloc_can_free;
    int free_can_alloc;
    int alloc_can_alloc;
    int free_can_free;
};

#define ALLOCMAN_DEFAULT_PROPERTIES { \
    .alloc_can_free = 0, \
    .free_can_alloc = 0, \
    .alloc_can_alloc = 0, \
    .free_can_free = 0 \
}

static const struct allocman_properties allocman_default_properties = {
    .alloc_can_free = 0,
    .free_can_alloc = 0,
    .alloc_can_alloc = 0,
    .free_can_free = 0
};


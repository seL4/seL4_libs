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
#include <sel4/sel4.h>

/* This is a delegating slab allocator which preallocates objects for
 * minimal cache collisions and maximal cache locality.
 *
 * Predefine the amount of objects and types required,
 * the allocator will sort them by size and
 * allocate from the start of an untyped, ensuring they
 * are adjacent.
 *
 * Initialise with another allocator to perform cspace allocation,
 * untyped allocation.
 *
 * When the slab allocator runs out, it will delegate any
 * further allocations.
 *
 * This allocator does not implement free, alloc_at, paddr or device related functions.
 */

/**
 * Initialise slab_vka
 *
 * @param slab_vka empty allocator to initialise
 * @param delegate initialised allocator to perform allocations with
 * @param object_freq frequency of objects required for slab allocation (indexed by object type).
 * @return 0 on success
 */
int slab_init(vka_t *slab_vka, vka_t *delegate, size_t object_freq[seL4_ObjectTypeCount]);

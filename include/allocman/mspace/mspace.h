/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _ALLOCMAN_MSPACE_H_
#define _ALLOCMAN_MSPACE_H_

#include <allocman/properties.h>

struct allocman;

struct mspace_interface {
    void *(*alloc)(struct allocman *alloc, void *cookie, uint32_t bytes, int *error);
    void (*free)(struct allocman *alloc, void *cookie, void *ptr, uint32_t bytes);
    struct allocman_properties properties;
    void *mspace;
};

#endif

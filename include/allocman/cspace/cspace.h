/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _ALLOCMAN_CSPACE_H_
#define _ALLOCMAN_CSPACE_H_

#include <sel4/types.h>
#include <allocman/properties.h>
#include <vka/cspacepath_t.h>

struct allocman;

typedef struct cspace_interface {
    int (*alloc)(struct allocman *alloc, void *cookie, cspacepath_t *path);
    void (*free)(struct allocman *alloc, void *cookie, cspacepath_t *path);
    cspacepath_t (*make_path)(void *cookie, seL4_CPtr slot);
    struct allocman_properties properties;
    void *cspace;
} cspace_interface_t;

#endif

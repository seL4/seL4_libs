/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _ALLOCMAN_CSPACE_SIMPLE1LEVEL_H_
#define _ALLOCMAN_CSPACE_SIMPLE1LEVEL_H_

#include <autoconf.h>
#include <sel4/types.h>
#include <allocman/cspace/cspace.h>

/* This is a very simple cspace allocator that monotinically allocates cptrs
   in a range, free is not possible. */
struct cspace_simple1level_config {
    /* A cptr to the cnode that we are managing slots in */
    seL4_CPtr cnode;
    /* Size in bits of the cnode */
    uint32_t cnode_size_bits;
    /* Guard depth added to this cspace. */
    uint32_t cnode_guard_bits;
    /* First valid slot (as an index) */
    uint32_t first_slot;
    /* Last valid slot + 1 (as an index) */
    uint32_t end_slot;
};

typedef struct cspace_simple1level {
    struct cspace_simple1level_config config;
    uint32_t current_slot;
} cspace_simple1level_t;

static inline cspacepath_t _cspace_simple1level_make_path(void *_cspace, seL4_CPtr slot)
{
    cspace_simple1level_t *cspace = (cspace_simple1level_t*)_cspace;
    return (cspacepath_t) {
        .root = cspace->config.cnode,
        .capPtr = slot,
        .offset = slot,
        .capDepth = cspace->config.cnode_size_bits + cspace->config.cnode_guard_bits,
        .dest = 0,
        .destDepth = 0,
        .window = 1
    };
}

/* Construction of this cspace is simplified by the fact it has a completely
   constant memory pool and is defined here. */
void cspace_simple1level_create(cspace_simple1level_t *cspace, struct cspace_simple1level_config config);

/* These functions are defined here largely so the interface definition can be
   made static, instead of returning it from a function. Also so that we can
   do away with this struct if using statically chosen allocators. cspaces are
   declared as void* to make type siginatures match up. I know it's ugly, but
   not my fault that this is the pattern for doing ADTs in C */
int _cspace_simple1level_alloc(struct allocman *alloc, void *_cspace, cspacepath_t *slot);
void _cspace_simple1level_free(struct allocman *alloc, void *_cspace, cspacepath_t *slot);

static inline struct cspace_interface cspace_simple1level_make_interface(cspace_simple1level_t *cspace) {
    return (struct cspace_interface){
        .alloc = _cspace_simple1level_alloc,
        .free = _cspace_simple1level_free,
        .make_path = _cspace_simple1level_make_path,
        /* We could give arbitrary recursion properties here. Since we perform no
           other allocations we never even have the potential to recurse */
        .properties = ALLOCMAN_DEFAULT_PROPERTIES,
        .cspace = cspace
    };
}

#endif

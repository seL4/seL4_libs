/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _ALLOCMAN_UTSPACE_SPLIT_H_
#define _ALLOCMAN_UTSPACE_SPLIT_H_

#include <autoconf.h>
#include <sel4/types.h>
#include <allocman/utspace/utspace.h>
#include <vka/cspacepath_t.h>
#include <assert.h>

/* This is an untyped manager that works by splitting each untyped in half to
 * create smaller untypeds. */

struct utspace_split_node {
    cspacepath_t ut;
    /* if this is a child node, represents our parent. Our parent must by
     * definition be considered allocated */
    struct utspace_split_node *parent;
    /* if we have a parent, then this is a pointer to our other sibling */
    struct utspace_split_node *sibling;
    /* whether or not this node is currrently in the free lists or not */
    int allocated;
    /* physical address of the node */
    uint32_t paddr;
    /* if this node is not allocated then these are the next/previous pointers in the free list */
    struct utspace_split_node *next, *prev;
};

typedef struct utspace_split {
    struct utspace_split_node *heads[32];
} utspace_split_t;

void utspace_split_create(utspace_split_t *split);
int _utspace_split_add_uts(struct allocman *alloc, void *_split, uint32_t num, cspacepath_t *uts, uint32_t *size_bits, uint32_t *paddr);

uint32_t _utspace_split_alloc(struct allocman *alloc, void *_split, uint32_t size_bits, seL4_Word type, cspacepath_t *slot, int *error);
void _utspace_split_free(struct allocman *alloc, void *_split, uint32_t cookie, uint32_t size_bits);

uint32_t _utspace_split_paddr(void *_split, uint32_t cookie, uint32_t size_bits);

static inline struct utspace_interface utspace_split_make_interface(utspace_split_t *split) {
    return (struct utspace_interface) {
        .alloc = _utspace_split_alloc,
        .free = _utspace_split_free,
        .add_uts = _utspace_split_add_uts,
        .paddr = _utspace_split_paddr,
        .properties = ALLOCMAN_DEFAULT_PROPERTIES,
        .utspace = split
    };
}

#endif

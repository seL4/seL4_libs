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
    /* which (if any) free list this is in */
    struct utspace_split_node **head;
    /* which free list this should go back into */
    struct utspace_split_node **origin_head;
    /* physical address of the node */
    uintptr_t paddr;
    /* if this node is not allocated then these are the next/previous pointers in the free list */
    struct utspace_split_node *next, *prev;
};

typedef struct utspace_split {
    /* untypeds from the kernel window. Used for anything */
    struct utspace_split_node *heads[CONFIG_WORD_SIZE];
    /* untypeds that are unknown device regions */
    struct utspace_split_node *dev_heads[CONFIG_WORD_SIZE];
    /* untypeds that are known to be RAM from the device region */
    struct utspace_split_node *dev_mem_heads[CONFIG_WORD_SIZE];
} utspace_split_t;

void utspace_split_create(utspace_split_t *split);
int _utspace_split_add_uts(struct allocman *alloc, void *_split, size_t num, const cspacepath_t *uts, size_t *size_bits, uintptr_t *paddr, int utType);

seL4_Word _utspace_split_alloc(struct allocman *alloc, void *_split, size_t size_bits, seL4_Word type, const cspacepath_t *slot, uintptr_t paddr, bool canBeDev, int *error);
void _utspace_split_free(struct allocman *alloc, void *_split, seL4_Word cookie, size_t size_bits);

uintptr_t _utspace_split_paddr(void *_split, seL4_Word cookie, size_t size_bits);

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


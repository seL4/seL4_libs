/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _ALLOCMAN_UTSPACE_TRICKLE_H_
#define _ALLOCMAN_UTSPACE_TRICKLE_H_

#include <autoconf.h>
#include <sel4/types.h>
#include <allocman/utspace/utspace.h>
#include <assert.h>

#ifdef CONFIG_KERNEL_STABLE

struct utspace_trickle_node {
    cspacepath_t *ut;
    uint32_t offset;
    uint32_t parent_cookie;
    uint32_t bitmap_bits;
    uint32_t bitmap;
    struct utspace_trickle_node *next, *prev;
    uint32_t padding;
    uint32_t paddr;
};

typedef struct utspace_trickle {
    struct utspace_trickle_node *heads[32];
} utspace_trickle_t;

void utspace_trickle_create(utspace_trickle_t *trickle);

int _utspace_trickle_add_uts(struct allocman *alloc, void *_trickle, uint32_t num, cspacepath_t *uts, uint32_t *size_bits, uint32_t *paddr);

uint32_t _utspace_trickle_alloc(struct allocman *alloc, void *_trickle, uint32_t size_bits, seL4_Word type, cspacepath_t *slot, int *error);
void _utspace_trickle_free(struct allocman *alloc, void *_trickle, uint32_t cookie, uint32_t size_bits);

uint32_t _utspace_trickle_paddr(void *_trickle, uint32_t cookie, uint32_t size_bits);

static inline utspace_interface_t utspace_trickle_make_interface(utspace_trickle_t *trickle) {
    return (utspace_interface_t) {
        .alloc = _utspace_trickle_alloc,
        .free = _utspace_trickle_free,
        .add_uts = _utspace_trickle_add_uts,
        .paddr = _utspace_trickle_paddr,
        .properties = ALLOCMAN_DEFAULT_PROPERTIES,
        .utspace = trickle
    };
}

#endif /* CONFIG_KERNEL_STABLE */

#endif

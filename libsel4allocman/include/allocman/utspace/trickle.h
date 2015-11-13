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
    size_t offset;
    seL4_Word parent_cookie;
    size_t bitmap_bits;
    size_t bitmap;
    struct utspace_trickle_node *next, *prev;
    size_t padding;
    uintptr_t paddr;
};

compile_time_assert(trickle_struct_size32, sizeof(struct utspace_trickle_node) == 36 || sizeof(size_t) == 8);
compile_time_assert(trickle_struct_size64, sizeof(struct utspace_trickle_node) == 72 || sizeof(size_t) == 4);

typedef struct utspace_trickle {
    struct utspace_trickle_node *heads[CONFIG_WORD_SIZE];
} utspace_trickle_t;

void utspace_trickle_create(utspace_trickle_t *trickle);

int _utspace_trickle_add_uts(struct allocman *alloc, void *_trickle, size_t num, const cspacepath_t *uts, size_t *size_bits, uintptr_t *paddr);

seL4_Word _utspace_trickle_alloc(struct allocman *alloc, void *_trickle, size_t size_bits, seL4_Word type, const cspacepath_t *slot, int *error);
void _utspace_trickle_free(struct allocman *alloc, void *_trickle, seL4_Word cookie, size_t size_bits);

uintptr_t _utspace_trickle_paddr(void *_trickle, seL4_Word cookie, size_t size_bits);

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

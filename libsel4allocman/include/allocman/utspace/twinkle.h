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

/* This is an untyped manager that is vaguely related to the twinkle allocator.
   This means it does a simple progressive allocation of untypeds and doesn't
   support free. */

struct utspace_twinkle_ut {
    cspacepath_t path;
    size_t offset;
    size_t size_bits;
};

typedef struct utspace_twinkle {
    size_t num_uts;
    struct utspace_twinkle_ut *uts;
} utspace_twinkle_t;

void utspace_twinkle_create(utspace_twinkle_t *twinkle);
int _utspace_twinkle_add_uts(struct allocman *alloc, void *_twinkle, size_t num, const cspacepath_t *uts, size_t *size_bits, uintptr_t *paddr, int utType);

seL4_Word _utspace_twinkle_alloc(struct allocman *alloc, void *_twinkle, size_t size_bits, seL4_Word type, const cspacepath_t *slot, uintptr_t paddr, bool canBeDev, int *error);
void _utspace_twinkle_free(struct allocman *alloc, void *_twinkle, seL4_Word cookie, size_t size_bits);

static inline uintptr_t _utspace_twinkle_paddr(void *_twinkle, seL4_Word cookie, size_t size_bits) {
    assert(!"not implemented");
    return ALLOCMAN_NO_PADDR;
}

static inline struct utspace_interface utspace_twinkle_make_interface(utspace_twinkle_t *twinkle) {
    return (struct utspace_interface) {
        .alloc = _utspace_twinkle_alloc,
        .free = _utspace_twinkle_free,
        .add_uts = _utspace_twinkle_add_uts,
        .paddr = _utspace_twinkle_paddr,
        .properties = ALLOCMAN_DEFAULT_PROPERTIES,
        .utspace = twinkle
    };
}


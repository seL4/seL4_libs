/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _ALLOCMAN_UTSPACE_TWINKLE_H_
#define _ALLOCMAN_UTSPACE_TWINKLE_H_

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
    uint32_t offset;
    uint32_t size_bits;
};

typedef struct utspace_twinkle {
    uint32_t num_uts;
    struct utspace_twinkle_ut *uts;
} utspace_twinkle_t;

void utspace_twinkle_create(utspace_twinkle_t *twinkle);
int _utspace_twinkle_add_uts(struct allocman *alloc, void *_twinkle, uint32_t num, cspacepath_t *uts, uint32_t *size_bits, uint32_t *paddr);

uint32_t _utspace_twinkle_alloc(struct allocman *alloc, void *_twinkle, uint32_t size_bits, seL4_Word type, cspacepath_t *slot, int *error);
void _utspace_twinkle_free(struct allocman *alloc, void *_twinkle, uint32_t cookie, uint32_t size_bits);

static inline uint32_t _utspace_twinkle_paddr(void *_twinkle, uint32_t cookie, uint32_t size_bits) {
    assert(!"not implemented");
    return 0;
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

#endif

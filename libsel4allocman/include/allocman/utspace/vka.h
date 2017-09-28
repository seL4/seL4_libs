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
#include <vka/vka.h>
#include <vka/kobject_t.h>
#include <assert.h>

/* This is an untyped manager that just proxies calls to a VKA interface.
 * We also need to do some extra book keeping work */

typedef struct utspace_vka_cookie {
    seL4_Word original_cookie;
    seL4_Word type;
} utspace_vka_cookie_t;

static inline seL4_Word _utspace_vka_alloc(struct allocman *alloc, void *_vka, size_t size_bits, seL4_Word type, const cspacepath_t *slot, uintptr_t paddr, bool canBeDevice, int *error)
{
    vka_t *vka = (vka_t *)_vka;
    size_t sel4_size_bits = get_sel4_object_size(type, size_bits);
    utspace_vka_cookie_t *cookie = (utspace_vka_cookie_t*)malloc(sizeof(*cookie));
    if (!cookie) {
        SET_ERROR(error, 1);
        return 0;
    }
    int _error;
    if (paddr == ALLOCMAN_NO_PADDR) {
        _error = vka_utspace_alloc(vka, slot, type, sel4_size_bits, &cookie->original_cookie);
    } else {
        _error = vka_utspace_alloc_at(vka, slot, type, sel4_size_bits, paddr, &cookie->original_cookie);
    }
    SET_ERROR(error, _error);
    if (!_error) {
        cookie->type = type;
    } else {
        free(cookie);
        cookie = NULL;
    }
    return (seL4_Word)cookie;
}

static inline void _utspace_vka_free(struct allocman *alloc, void *_vka, seL4_Word _cookie, size_t size_bits)
{
    vka_t *vka = (vka_t *)_vka;
    utspace_vka_cookie_t *cookie = (utspace_vka_cookie_t*)_cookie;
    vka_utspace_free(vka, cookie->original_cookie, cookie->type, get_sel4_object_size(cookie->type, size_bits));
}

static inline uintptr_t _utspace_vka_paddr(void *_vka, seL4_Word _cookie, size_t size_bits)
{
    vka_t *vka = (vka_t *)_vka;
    utspace_vka_cookie_t *cookie = (utspace_vka_cookie_t*)_cookie;
    return vka_utspace_paddr(vka, cookie->original_cookie, cookie->type, get_sel4_object_size(cookie->type, size_bits));
}

static inline int _utspace_vka_add_uts(struct allocman *alloc, void *_trickle, size_t num, const cspacepath_t *uts, size_t *size_bits, uintptr_t *paddr, int utType)
{
    assert(!"VKA interface does not support adding untypeds after creation");
    return -1;
}

/**
 * Make a utspace interface from a VKA. It is the responsibility of the caller to ensure
 * that this pointer remains valid for as long as this cspace is used
 *
 * @param vka Allocator to proxy cspace calls to
 * @return utspace_interface that can be given to allocman
 */
static inline struct utspace_interface utspace_vka_make_interface(vka_t *vka) {
    return (struct utspace_interface) {
        .alloc = _utspace_vka_alloc,
        .free = _utspace_vka_free,
        .add_uts = _utspace_vka_add_uts,
        .paddr = _utspace_vka_paddr,
        .properties = ALLOCMAN_DEFAULT_PROPERTIES,
        .utspace = vka
    };
}


/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _ALLOCMAN_UTSPACE_VKA_H_
#define _ALLOCMAN_UTSPACE_VKA_H_

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
    uint32_t original_cookie;
    seL4_Word type;
} utspace_vka_cookie_t;

static inline uint32_t _utspace_vka_alloc(struct allocman *alloc, void *_vka, uint32_t size_bits, seL4_Word type, cspacepath_t *slot, int *error)
{
    vka_t *vka = (vka_t *)_vka;
    int sel4_size_bits = get_sel4_object_size(type, size_bits);
    utspace_vka_cookie_t *cookie = (utspace_vka_cookie_t*)malloc(sizeof(*cookie));
    if (!cookie) {
        SET_ERROR(error, 1);
        return 0;
    }
    int _error = vka_utspace_alloc(vka, slot, type, sel4_size_bits, &cookie->original_cookie);
    SET_ERROR(error, _error);
    if (!_error) {
        cookie->type = type;
    } else {
        free(cookie);
        cookie = NULL;
    }
    return (uint32_t)cookie;
}

static inline void _utspace_vka_free(struct allocman *alloc, void *_vka, uint32_t _cookie, uint32_t size_bits)
{
    vka_t *vka = (vka_t *)_vka;
    utspace_vka_cookie_t *cookie = (utspace_vka_cookie_t*)_cookie;
    vka_utspace_free(vka, cookie->original_cookie, cookie->type, get_sel4_object_size(cookie->type, size_bits));
}

static inline uint32_t _utspace_vka_paddr(void *_vka, uint32_t _cookie, uint32_t size_bits)
{
    vka_t *vka = (vka_t *)_vka;
    utspace_vka_cookie_t *cookie = (utspace_vka_cookie_t*)_cookie;
    return vka_utspace_paddr(vka, cookie->original_cookie, cookie->type, get_sel4_object_size(cookie->type, size_bits));
}

static inline int _utspace_vka_add_uts(struct allocman *alloc, void *_trickle, uint32_t num, cspacepath_t *uts, uint32_t *size_bits, uint32_t *paddr)
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

#endif

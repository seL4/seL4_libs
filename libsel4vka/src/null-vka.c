/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <assert.h>
#include <sel4/sel4.h>
#include <stdint.h>
#include <stdlib.h>
#include <vka/cspacepath_t.h>
#include <vka/null-vka.h>
#include <vka/vka.h>

static int cspace_alloc(void *data, seL4_CPtr *res)
{
    return -1;
}

static void cspace_make_path(void *data, seL4_CPtr slot, cspacepath_t *res)
{
}

static void cspace_free(void *data, seL4_CPtr slot)
{
}

static int utspace_alloc(void *data, const cspacepath_t *dest, seL4_Word type,
                         seL4_Word size_bits, seL4_Word *res)
{
    return -1;
}

static int utspace_alloc_maybe_device(void *data, const cspacepath_t *dest, seL4_Word type,
                         seL4_Word size_bits, bool can_use_dev, seL4_Word *res)
{
    return -1;
}

static int utspace_alloc_at(void *data, const cspacepath_t *dest, seL4_Word type,
                            seL4_Word size_bits, uintptr_t paddr, seL4_Word *res)
{
    return -1;
}

static void utspace_free(void *data, seL4_Word type, seL4_Word size_bits,
                         seL4_Word target)
{
}

static uintptr_t utspace_paddr(void *data, seL4_Word target, seL4_Word type, seL4_Word size_bits)
{
    return VKA_NO_PADDR;
}

void vka_init_nullvka(vka_t *vka)
{
    assert(vka != NULL);
    *vka = (vka_t) {
        .data = NULL, /* not required */
        .cspace_alloc = cspace_alloc,
        .cspace_make_path = cspace_make_path,
        .utspace_alloc = utspace_alloc,
        .utspace_alloc_maybe_device = utspace_alloc_maybe_device,
        .utspace_alloc_at = utspace_alloc_at,
        .cspace_free = cspace_free,
        .utspace_free = utspace_free,
        .utspace_paddr = utspace_paddr
    };
}

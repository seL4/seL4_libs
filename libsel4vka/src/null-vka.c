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

void vka_init_nullvka(vka_t *vka)
{
    assert(vka != NULL);
    vka->data = NULL; /* not required */
    vka->cspace_alloc = cspace_alloc;
    vka->cspace_make_path = cspace_make_path;
    vka->utspace_alloc = utspace_alloc;
    vka->utspace_alloc_maybe_device = utspace_alloc_maybe_device;
    vka->utspace_alloc_at = utspace_alloc_at;
    vka->cspace_free = cspace_free;
    vka->utspace_free = utspace_free;
}

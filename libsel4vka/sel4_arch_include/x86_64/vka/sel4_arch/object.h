/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <vka/vka.h>
#include <vka/kobject_t.h>
#include <utils/util.h>

static inline int vka_alloc_pml4(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, kobject_get_type(KOBJECT_PML4, 0), seL4_PML4Bits, result);
}

static inline int vka_alloc_pdpt(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, kobject_get_type(KOBJECT_PDPT, 0), seL4_PDPTBits, result);
}

LEAKY(pml4)
LEAKY(pdpt)

static inline int vka_alloc_vspace_root(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_pml4(vka, result);
}

static inline unsigned long
vka_x86_mode_get_object_size(seL4_Word objectType)
{
    switch (objectType) {
    case seL4_X64_HugePageObject:
        return seL4_HugePageBits;
    case seL4_X86_PDPTObject:
        return seL4_PDPTBits;
    case seL4_X64_PML4Object:
        return seL4_PML4Bits;
    default:
        /* Unknown object type. */
        ZF_LOGE("Unknown object type %ld", (long)objectType);
        return -1;
    }
}


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

#include <vka/vka.h>
#include <vka/kobject_t.h>
#include <utils/util.h>

static inline int vka_alloc_pdpt(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, kobject_get_type(KOBJECT_PDPT, 0), seL4_PDPTBits, result);
}

LEAKY(pdpt)

static inline unsigned long
vka_x86_mode_get_object_size(seL4_Word objectType)
{
    switch (objectType) {
    case seL4_IA32_PDPTObject:
        return seL4_PDPTBits;
    default:
        /* Unknown object type. */
        ZF_LOGF("Unknown object type %ld", (long)objectType);
        return -1;
    }
}

static inline int vka_alloc_vspace_root(vka_t *vka, vka_object_t *result)
{
#ifdef CONFIG_PAE_PAGING
    return vka_alloc_pdpt(vka, result);
#else
    return vka_alloc_page_directory(vka, result);
#endif
}


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
#include <sel4vka/gen_config.h>
#include <vka/vka.h>
#include <vka/kobject_t.h>
#include <utils/util.h>

static inline int vka_alloc_vspace_root(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_page_directory(vka, result);
}

/*
 * Get the size (in bits) of the untyped memory required to create an object of
 * the given size.
 */
static inline unsigned long vka_arm_mode_get_object_size(seL4_Word objectType)
{
    switch (objectType) {
    case seL4_ARM_SectionObject:
        return seL4_SectionBits;
    case seL4_ARM_SuperSectionObject:
        return seL4_SuperSectionBits;
    default:
        /* Unknown object type. */
        ZF_LOGE("Unknown object type");
        return -1;
    }
}


/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
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


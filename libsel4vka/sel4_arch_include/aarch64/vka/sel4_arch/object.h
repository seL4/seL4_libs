/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <vka/vka.h>
#include <vka/kobject_t.h>
#include <utils/util.h>

static inline int vka_alloc_page_global_directory(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, kobject_get_type(KOBJECT_PAGE_GLOBAL_DIRECTORY, 0), seL4_PGDBits, result);
}

static inline int vka_alloc_page_upper_directory(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, kobject_get_type(KOBJECT_PAGE_UPPER_DIRECTORY, 0), seL4_PUDBits, result);
}

LEAKY(page_global_directory)
LEAKY(page_upper_directory)

static inline int vka_alloc_vspace_root(vka_t *vka, vka_object_t *result)
{
    if (config_set(CONFIG_ARM_HYPERVISOR_SUPPORT) && config_set(CONFIG_ARM_PA_SIZE_BITS_40)) {
        return vka_alloc_page_upper_directory(vka, result);
    } else {
        return vka_alloc_page_global_directory(vka, result);
    }
}

/*
 * Get the size (in bits) of the untyped memory required to create an object of
 * the given size.
 */
static inline unsigned long
vka_arm_mode_get_object_size(seL4_Word objectType)
{
    switch (objectType) {
    case seL4_ARM_HugePageObject:
        return seL4_HugePageBits;
    case seL4_ARM_PageGlobalDirectoryObject:
        return seL4_PGDBits;
    case seL4_ARM_PageUpperDirectoryObject:
        return seL4_PUDBits;
    default:
        /* Unknown object type. */
        ZF_LOGE("Unknown object type");
        return -1;
    }
}


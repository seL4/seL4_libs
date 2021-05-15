/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sel4/types.h>
#include <assert.h>
#include <autoconf.h>
#include <sel4vka/gen_config.h>
#include <utils/util.h>
#include <vka/sel4_arch/kobject_t.h>

enum _arm_kobject_type {
    KOBJECT_PAGE_DIRECTORY = KOBJECT_MODE_NUM_TYPES,
    KOBJECT_PAGE_TABLE,
    KOBJECT_ARCH_NUM_TYPES,
};

/*
 * Get the size (in bits) of the untyped memory required to
 * create an object of the given size
 */
static inline seL4_Word arch_kobject_get_size(kobject_t type, seL4_Word objectSize)
{
    switch (type) {
    /* ARM-specific frames. */
    case KOBJECT_FRAME:
        switch (objectSize) {
        case seL4_PageBits:
        case seL4_LargePageBits:
            return objectSize;
        }
    /* If frame size was unknown fall through to default case as it
     * might be a mode specific frame size */
    default:
        return arm_mode_kobject_get_size(type, objectSize);
    }
}

static inline seL4_Word arch_kobject_get_type(kobject_t type, seL4_Word objectSize)
{
    switch (type) {
    case KOBJECT_PAGE_DIRECTORY:
        return seL4_ARM_PageDirectoryObject;
    case KOBJECT_PAGE_TABLE:
        return seL4_ARM_PageTableObject;
    /* ARM-specific frames. */
    case KOBJECT_FRAME:
        switch (objectSize) {
        case seL4_PageBits:
            return seL4_ARM_SmallPageObject;
        case seL4_LargePageBits:
            return seL4_ARM_LargePageObject;
        default:
            return arm_mode_kobject_get_type(type, objectSize);
        }
    default:
        return arm_mode_kobject_get_type(type, objectSize);
    }
}


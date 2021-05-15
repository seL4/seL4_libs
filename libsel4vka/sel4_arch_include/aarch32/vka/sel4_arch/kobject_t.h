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

enum _arm_mode_kobject_type {
    KOBJECT_FRAME = 0,
    KOBJECT_MODE_NUM_TYPES,
};

typedef int kobject_t;

/*
 * Get the size (in bits) of the untyped memory required to
 * create an object of the given size
 */
static inline seL4_Word arm_mode_kobject_get_size(kobject_t type, seL4_Word objectSize)
{
    switch (type) {
    /* ARM-specific frames. */
    case KOBJECT_FRAME:
        switch (objectSize) {
        case seL4_SectionBits:
        case seL4_SuperSectionBits:
            return objectSize;
        default:
            return 0;
        }
    default:
        /* Unknown object type. */
        ZF_LOGE("Unknown object type");
        return 0;
    }
}

static inline seL4_Word arm_mode_kobject_get_type(kobject_t type, seL4_Word objectSize)
{
    switch (type) {
    case KOBJECT_FRAME:
        switch (objectSize) {
        case seL4_SectionBits:
            return seL4_ARM_SectionObject;
        case seL4_SuperSectionBits:
            return seL4_ARM_SuperSectionObject;
        default:
            return -1;
        }
    default:
        /* Unknown object type. */
        ZF_LOGE("Unknown object type");
        return -1;
    }
}


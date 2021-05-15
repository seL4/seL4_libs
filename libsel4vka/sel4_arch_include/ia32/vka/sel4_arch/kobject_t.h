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

enum _x86_mode_kobject_type {
    KOBJECT_PDPT,
    KOBJECT_FRAME,
    KOBJECT_MODE_NUM_TYPES,
};

typedef int kobject_t;

/*
 * Get the size (in bits) of the untyped memory required to
 * create an object of the given size.
 */
static inline seL4_Word x86_mode_kobject_get_size(kobject_t type, seL4_Word UNUSED objectSize)
{
    switch (type) {
    case KOBJECT_PDPT:
        return seL4_PDPTBits;
    default:
        ZF_LOGE("Unknown object type");
        return 0;
    }
}

static inline seL4_Word x86_mode_kobject_get_type(kobject_t type, seL4_Word UNUSED objectSize)
{
    switch (type) {
    case KOBJECT_PDPT:
        return seL4_IA32_PDPTObject;
    default:
        /* Unknown object type. */
        ZF_LOGE("Unknown object type %d", type);
        return -1;
    }
}


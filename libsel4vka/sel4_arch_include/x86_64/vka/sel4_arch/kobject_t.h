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

#include <sel4/types.h>
#include <assert.h>
#include <autoconf.h>
#include <utils/util.h>

enum _x86_mode_kobject_type {
    KOBJECT_PML4,
    KOBJECT_PDPT,
    KOBJECT_FRAME,
    KOBJECT_MODE_NUM_TYPES,
};

typedef int kobject_t;

/*
 * Get the size (in bits) of the untyped memory required to
 * create an object of the given size.
 */
static inline seL4_Word
x86_mode_kobject_get_size(kobject_t type, seL4_Word objectSize)
{
    switch (type) {
    case KOBJECT_FRAME:
        switch (objectSize) {
        case seL4_HugePageBits:
            return objectSize;
        default:
            return 0;
        }
    case KOBJECT_PDPT:
        return seL4_PDPTBits;
    default:
        ZF_LOGE("Uknown object type");
        return 0;
    }
}

static inline seL4_Word
x86_mode_kobject_get_type(kobject_t type, seL4_Word objectSize)
{
    switch (type) {
    case KOBJECT_FRAME:
        switch (objectSize) {
        case seL4_HugePageBits:
            return seL4_X64_HugePageObject;
        default:
            return -1;
        }
    case KOBJECT_PML4:
        return seL4_X64_PML4Object;
    case KOBJECT_PDPT:
        return seL4_X86_PDPTObject;
    default:
        /* Unknown object type. */
        ZF_LOGE("Unknown object type %d", type);
        return -1;
    }
}


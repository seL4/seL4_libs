/*
 * Copyright 2018, Data61
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

#define seL4_PageDirBits seL4_PageTableBits

enum _riscv_kobject_type {
    KOBJECT_PAGE_DIRECTORY,
    KOBJECT_PAGE_TABLE,
    KOBJECT_FRAME,
    KOBJECT_ARCH_NUM_TYPES,
};
typedef int kobject_t;

/*
 * Get the size (in bits) of the untyped memory required to
 * create an object of the given size.
 */
static inline seL4_Word
arch_kobject_get_size(kobject_t type, seL4_Word objectSize)
{
    switch (type) {
    case KOBJECT_FRAME:
        switch (objectSize) {
        case seL4_PageBits:
        case seL4_LargePageBits:
            return objectSize;
        }
        /* If frame size was unknown fall through to default case as it
         * might be a mode specific frame size */
    default:
        ZF_LOGE("Uknown object type");
        return 0;
    }
}


static inline seL4_Word
arch_kobject_get_type(int type, seL4_Word objectSize)
{
    switch (type) {
    case KOBJECT_PAGE_DIRECTORY:
    case KOBJECT_PAGE_TABLE:
        return seL4_RISCV_PageTableObject;
    case KOBJECT_FRAME:
        switch (objectSize) {
        case seL4_PageBits:
            return seL4_RISCV_4K_Page;
        case seL4_LargePageBits:
            return seL4_RISCV_Mega_Page;
#if CONFIG_PT_LEVELS > 2
        case seL4_HugePageBits:
            return seL4_RISCV_Giga_Page;
#endif
#if CONFIG_PT_LEVELS > 3
        case seL4_TeraPageBits:
            return seL4_RISCV_Tera_Page;
#endif
        default:
            ZF_LOGE("Unknown frame size %zu", (size_t) objectSize);
            return -1;
        }
    default:
          ZF_LOGE("Unknown object type %d", type);
          return -1;
    }
}



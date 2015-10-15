/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _ARCH_KOBJECT_T_H_
#define _ARCH_KOBJECT_T_H_

#include <sel4/types.h>
#include <assert.h>
#include <autoconf.h>
#include <utils/util.h>

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
        case 21:
            return objectSize;
        case 22:
            return objectSize;
        default:
            return 0;
        }
        return seL4_PageBits;
#ifdef CONFIG_IOMMU
    case KOBJECT_IO_PAGETABLE:
        return seL4_IOPageTableBits;
#endif /* CONFIG_IOMMU */
    default:
        /* Unknown object type. */
        ZF_LOGE("Unknown object type");
        return 0;
    }
}

static inline seL4_Word
kobject_get_type(kobject_t type, seL4_Word objectSize)
{
    switch (type) {
    case KOBJECT_PAGE_MAP_LEVEL4:
        return seL4_X64_PageMapLevel4Object;
    case KOBJECT_PAGE_DIRECTORY_POINTER_TABLE:
        return seL4_X64_PageDirectoryPointerTableObject;
    case KOBJECT_PAGE_DIRECTORY:
        return seL4_X64_PageDirectoryObject;
    case KOBJECT_PAGE_TABLE:
        return seL4_X64_PageTableObject;
    case KOBJECT_FRAME:
        switch (objectSize) {
        case seL4_PageBits:
            return seL4_X64_4K;
        case 21:
            return seL4_X64_2M;
        default:
            return -1;
        }
    case KOBJECT_IO_PAGETABLE:
        return seL4_X64_IOPageTableObject;
    default:
        ZF_LOGE("Unknown object type");
        return -1;
    }
}

#endif /* _ARCH_KOBJECT_T_H_ */

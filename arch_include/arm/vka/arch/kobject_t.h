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
        /* ARM-specific frames. */
    case KOBJECT_FRAME:
        switch (objectSize) {
        case seL4_PageBits:
        case 16:
        case 20:
        case 24:
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


static inline seL4_Word
arch_kobject_get_type(kobject_t type, seL4_Word objectSize)
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
        case 16:
            return seL4_ARM_LargePageObject;
#if defined(ARM_HYP)
        case 21:
            return seL4_ARM_SectionObject;
        case 25:
            return seL4_ARM_SuperSectionObject;
#else
        case 20:
            return seL4_ARM_SectionObject;
        case 24:
            return seL4_ARM_SuperSectionObject;
#endif
        default:
            ZF_LOGE("Unknown object type");
            return -1;
        }
    default:
        ZF_LOGE("Unknown object type");
        return -1;
    }
}

#endif /* _ARCH_KOBJECT_T_H_ */

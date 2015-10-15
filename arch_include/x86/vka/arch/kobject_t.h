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
        ZF_LOGE("Uknown object type");
        return 0;
    }
}


static inline seL4_Word
arch_kobject_get_type(kobject_t type, seL4_Word objectSize)
{
    switch (type) {
    case KOBJECT_PAGE_DIRECTORY:
        return seL4_IA32_PageDirectoryObject;
    case KOBJECT_PAGE_TABLE:
        return seL4_IA32_PageTableObject;
    case KOBJECT_FRAME:
        switch (objectSize) {
        case seL4_PageBits:
            return seL4_IA32_4K;
            /* Use an #ifdef here to support any old kernels that might
             * not have seL4_LargePageBits defined. This should be able
             * to be dropped eventually */
#ifdef CONFIG_PAE_PAGING
        case seL4_LargePageBits:
            return seL4_IA32_LargePage;
#else
        case 22:
            return seL4_IA32_4M;
#endif /* CONFIG_PAE_PAGING */
        default:
            return -1;
        }
#ifdef CONFIG_IOMMU
    case KOBJECT_IO_PAGETABLE:
        return seL4_IA32_IOPageTableObject;
#endif /* CONFIG_IOMMU */
    default:
        /* Unknown object type. */
        ZF_LOGE("Unknown object type");
        return -1;
    }
}


#endif /* _ARCH_KOBJECT_T_H_ */

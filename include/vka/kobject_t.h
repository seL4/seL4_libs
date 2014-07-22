/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _KOBJECT_T_H_
#define _KOBJECT_T_H_

#include <sel4/types.h>
#include <assert.h>
#include <autoconf.h>

/* Generic Kernel Object Type used by generic allocator */

typedef enum _kobject_type {
    KOBJECT_TCB,
    KOBJECT_CNODE,
    KOBJECT_CSLOT,
    KOBJECT_UNTYPED,
    KOBJECT_ENDPOINT_SYNC,
    KOBJECT_ENDPOINT_ASYNC,
    KOBJECT_FRAME,
    KOBJECT_PAGE_DIRECTORY,
    KOBJECT_PAGE_TABLE,
#ifdef CONFIG_X86_64
    KOBJECT_PAGE_MAP_LEVEL4,
    KOBJECT_PAGE_DIRECTORY_POINTER_TABLE,
#endif
#ifdef CONFIG_IOMMU
    KOBJECT_IO_PAGETABLE,
#endif
#ifdef CONFIG_CACHE_COLORING 
    KOBJECT_KERNEL_IMAGE,
#endif 
    NUM_KOBJECT_TYPES

} kobject_t;


/*
 * Get the size (in bits) of the untyped memory required to
 * create an object of the given size.
 */
static inline seL4_Word
kobject_get_size(kobject_t type, seL4_Word objectSize)
{
    switch (type) {
        /* Generic objects. */
    case KOBJECT_TCB:
        return seL4_TCBBits;
    case KOBJECT_CNODE:
        return (seL4_SlotBits + objectSize);
    case KOBJECT_CSLOT:
        return 0;
    case KOBJECT_UNTYPED:
        return objectSize;
    case KOBJECT_ENDPOINT_SYNC:
        return seL4_EndpointBits;
    case KOBJECT_ENDPOINT_ASYNC:
        return seL4_EndpointBits;
    case KOBJECT_PAGE_DIRECTORY:
        return seL4_PageDirBits;
    case KOBJECT_PAGE_TABLE:
        return seL4_PageTableBits;
#ifdef CONFIG_CACHE_COLORING 
    case KOBJECT_KERNEL_IMAGE:
        return seL4_KernelImageBits;
#endif 
#if defined(CONFIG_ARCH_ARM)
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
#elif defined(CONFIG_ARCH_I386)
        /* IA32-specific frames */
    case KOBJECT_FRAME:
        switch (objectSize) {
        case seL4_PageBits:
#ifdef CONFIG_X86_64
        case 21:
            return objectSize;
#endif
        case 22:
            return objectSize;
        default:
            return 0;
        }
        return seL4_PageBits;
        /* IA32-specific object */
#ifdef CONFIG_IOMMU
    case KOBJECT_IO_PAGETABLE:
        return seL4_IOPageTableBits;
#endif
#else
#error "Unknown arch."
#endif
    default:
        /* Unknown object type. */
        assert(0);
        return 0;
    }
}


static inline seL4_Word
kobject_get_type(kobject_t type, seL4_Word objectSize)
{
    switch (type) {
        /* Generic objects. */
    case KOBJECT_TCB:
        return seL4_TCBObject;
    case KOBJECT_CNODE:
        return seL4_CapTableObject;
    case KOBJECT_CSLOT:
        return -1;
    case KOBJECT_UNTYPED:
        return seL4_UntypedObject;
    case KOBJECT_ENDPOINT_SYNC:
        return seL4_EndpointObject;
    case KOBJECT_ENDPOINT_ASYNC:
        return seL4_AsyncEndpointObject;
#ifdef CONFIG_CACHE_COLORING 
    case KOBJECT_KERNEL_IMAGE:
        return seL4_KernelImageObject;
#endif 
#if defined(CONFIG_ARCH_ARM)
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
            return -1;
        }
#elif defined(CONFIG_ARCH_I386)
#ifdef CONFIG_X86_64
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
#else
        /* IA32-specific frames */
    case KOBJECT_PAGE_DIRECTORY:
        return seL4_IA32_PageDirectoryObject;
    case KOBJECT_PAGE_TABLE:
        return seL4_IA32_PageTableObject;
    case KOBJECT_FRAME:
        switch (objectSize) {
        case seL4_PageBits:
            return seL4_IA32_4K;
        case 22:
            return seL4_IA32_4M;
        default:
            return -1;
        }
#endif
        /* IA32-specific object */
#ifdef CONFIG_IOMMU
    case KOBJECT_IO_PAGETABLE:
#ifdef CONFIG_X86_64
        return seL4_X64_IOPageTableObject;
#else
        return seL4_IA32_IOPageTableObject;
#endif
#endif
#else
#error "Unknown arch."
#endif
    default:
        /* Unknown object type. */
        assert(0);
        return -1;
    }
}


#endif /* _KOBJECT_T_H_ */

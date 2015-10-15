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
#include <utils/util.h>

/* Generic Kernel Object Type used by generic allocator */

typedef enum _kobject_type {
    KOBJECT_TCB,
    KOBJECT_CNODE,
    KOBJECT_CSLOT,
    KOBJECT_UNTYPED,
    KOBJECT_ENDPOINT,
    KOBJECT_NOTIFICATION,
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
DEPRECATED("Use KOBJECT_NOTIFICATION") static const kobject_t KOBJECT_ASYNC_ENDPOINT = KOBJECT_NOTIFICATION;
DEPRECATED("Use KOBJECT_ENDPOINT") static const kobject_t KOBJECT_SYNC_ENDPOINT  = KOBJECT_ENDPOINT;

#include <vka/arch/kobject_t.h>
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
    case KOBJECT_ENDPOINT:
        return seL4_EndpointBits;
    case KOBJECT_NOTIFICATION:
        return seL4_EndpointBits;
    case KOBJECT_PAGE_DIRECTORY:
        return seL4_PageDirBits;
    case KOBJECT_PAGE_TABLE:
        return seL4_PageTableBits;
#ifdef CONFIG_CACHE_COLORING
    case KOBJECT_KERNEL_IMAGE:
        return seL4_KernelImageBits;
#endif
    default:
        return arch_kobject_get_size(type, objectSize);
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
    case KOBJECT_ENDPOINT:
        return seL4_EndpointObject;
    case KOBJECT_NOTIFICATION:
        return seL4_NotificationObject;
#ifdef CONFIG_CACHE_COLORING
    case KOBJECT_KERNEL_IMAGE:
        return seL4_KernelImageObject;
#endif
    default:
        return arch_kobject_get_type(type, objectSize);
    }
}


#endif /* _KOBJECT_T_H_ */

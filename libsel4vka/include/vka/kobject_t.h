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
#include <vka/arch/kobject_t.h>

/* Generic Kernel Object Type used by generic allocator */

enum _kobject_type {
    KOBJECT_TCB = KOBJECT_ARCH_NUM_TYPES,
    KOBJECT_CNODE,
    KOBJECT_CSLOT,
    KOBJECT_UNTYPED,
    KOBJECT_ENDPOINT,
    KOBJECT_NOTIFICATION,
    KOBJECT_REPLY,
    KOBJECT_SCHED_CONTEXT,
#ifdef CONFIG_CACHE_COLORING
    KOBJECT_KERNEL_IMAGE,
#endif
    NUM_KOBJECT_TYPES

};

DEPRECATED("Use KOBJECT_NOTIFICATION") static const kobject_t KOBJECT_ASYNC_ENDPOINT = KOBJECT_NOTIFICATION;
DEPRECATED("Use KOBJECT_ENDPOINT") static const kobject_t KOBJECT_SYNC_ENDPOINT  = KOBJECT_ENDPOINT;

/*
 * Get the size (in bits) of the untyped memory required to
 * create an object of the given size.
 */
static inline seL4_Word kobject_get_size(kobject_t type, seL4_Word objectSize)
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
#ifdef CONFIG_KERNEL_MCS
    case KOBJECT_REPLY:
        return seL4_ReplyBits;
    case KOBJECT_SCHED_CONTEXT:
        return objectSize > seL4_MinSchedContextBits ? objectSize : seL4_MinSchedContextBits;
#endif
#ifdef CONFIG_CACHE_COLORING
    case KOBJECT_KERNEL_IMAGE:
        return seL4_KernelImageBits;
#endif
    default:
        return arch_kobject_get_size(type, objectSize);
    }
}

static inline seL4_Word kobject_get_type(kobject_t type, seL4_Word objectSize)
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
#ifdef CONFIG_KERNEL_MCS
    case KOBJECT_SCHED_CONTEXT:
        return seL4_SchedContextObject;
    case KOBJECT_REPLY:
        return seL4_ReplyObject;
#endif
#ifdef CONFIG_CACHE_COLORING
    case KOBJECT_KERNEL_IMAGE:
        return seL4_KernelImageObject;
#endif
    default:
        return arch_kobject_get_type(type, objectSize);
    }
}


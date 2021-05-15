/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <assert.h>
#include <errno.h>
#include <vka/vka.h>
#include <vka/kobject_t.h>
#include <stdio.h>
#include <string.h>
#include <utils/util.h>

/*
 * A wrapper to hold all the allocation information for an 'object'
 * An object here is just combination of cptr and untyped allocation
 * The type and size of the allocation is also stored to make free
 * more convenient.
 */

typedef struct vka_object {
    seL4_CPtr cptr;
    seL4_Word ut;
    seL4_Word type;
    seL4_Word size_bits;
} vka_object_t;

/*
 * Generic object allocator used by functions below, can also be used directly
 */
static inline int vka_alloc_object_at_maybe_dev(vka_t *vka, seL4_Word type, seL4_Word size_bits, uintptr_t paddr,
                                                bool can_use_dev, vka_object_t *result)
{
    int error = -1;
    if (!(type < seL4_ObjectTypeCount)) {
        result->cptr = 0;
        ZF_LOGE("Unknown object type: %ld", (long) type);
        goto error;
    }

    error = vka_cspace_alloc(vka, &result->cptr);
    if (unlikely(error)) {
        result->cptr = 0;
        ZF_LOGE("Failed to allocate cslot: error %d", error);
        goto error;
    }

    cspacepath_t path;
    vka_cspace_make_path(vka, result->cptr, &path);

    if (paddr == VKA_NO_PADDR) {
        error = vka_utspace_alloc_maybe_device(vka, &path, type, size_bits, can_use_dev, &result->ut);
        if (unlikely(error)) {
            ZF_LOGE("Failed to allocate object of size %lu, error %d",
                    BIT(size_bits), error);
            goto error;
        }
    } else {
        error = vka_utspace_alloc_at(vka, &path, type, size_bits, paddr, &result->ut);
        if (unlikely(error)) {
            ZF_LOGE("Failed to allocate object of size %lu at paddr %p, error %d",
                    BIT(size_bits), (void *)paddr, error);
            goto error;
        }
    }

    result->type = type;
    result->size_bits = size_bits;
    return 0;

error:
    if (result->cptr) {
        vka_cspace_free(vka, result->cptr);
    }
    /* don't return garbage to the caller */
    memset(result, 0, sizeof(*result));
    return error;
}

static inline int vka_alloc_object_at(vka_t *vka, seL4_Word type, seL4_Word size_bits, uintptr_t paddr,
                                      vka_object_t *result)
{
    return vka_alloc_object_at_maybe_dev(vka, type, size_bits, paddr, false, result);
}
static inline int vka_alloc_object(vka_t *vka, seL4_Word type, seL4_Word size_bits, vka_object_t *result)
{
    return vka_alloc_object_at(vka, type, size_bits, VKA_NO_PADDR, result);
}

/* convenient wrapper that throws away the vka_object_t and just returns the cptr -
 * note you cannot use this if you intend to free the object */
static inline seL4_CPtr vka_alloc_object_leaky(vka_t *vka, seL4_Word type, seL4_Word size_bits) WARN_UNUSED_RESULT;
static inline seL4_CPtr vka_alloc_object_leaky(vka_t *vka, seL4_Word type, seL4_Word size_bits)
{
    vka_object_t result = {.cptr = 0, .ut = 0, .type = 0, size_bits = 0};
    return vka_alloc_object(vka, type, size_bits, &result) == -1 ? 0 : result.cptr;
}

static inline void vka_free_object(vka_t *vka, vka_object_t *object)
{
    cspacepath_t path;
    vka_cspace_make_path(vka, object->cptr, &path);

    if (path.capPtr == 0) {
        ZF_LOGE("Failed to create cspace path to object");
        return;
    }

    /* ignore any errors */
    seL4_CNode_Delete(path.root, path.capPtr, path.capDepth);

    vka_cspace_free(vka, object->cptr);
    vka_utspace_free(vka, object->type, object->size_bits, object->ut);
}

static inline uintptr_t vka_object_paddr(vka_t *vka, vka_object_t *object)
{
    return vka_utspace_paddr(vka, object->ut, object->type, object->size_bits);
}

/* Convenience wrappers for allocating objects */
static inline int vka_alloc_untyped(vka_t *vka, uint32_t size_bits, vka_object_t *result)
{
    return vka_alloc_object(vka, seL4_UntypedObject, size_bits, result);
}

static inline int vka_alloc_untyped_at(vka_t *vka, uint32_t size_bits, uintptr_t paddr,
                                       vka_object_t *result)
{
    return vka_alloc_object_at(vka, seL4_UntypedObject, size_bits, paddr, result);
}

static inline int vka_alloc_tcb(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, seL4_TCBObject, seL4_TCBBits, result);
}

static inline int vka_alloc_sched_context(UNUSED vka_t *vka, UNUSED vka_object_t *result)
{
#ifdef CONFIG_KERNEL_MCS
    return vka_alloc_object(vka, seL4_SchedContextObject, seL4_MinSchedContextBits, result);
#else
    ZF_LOGW("Allocating sched context on non RT kernel");
    return ENOSYS;
#endif
}

static inline int vka_alloc_sched_context_size(UNUSED vka_t *vka, UNUSED vka_object_t *result,
                                               UNUSED uint32_t size_bits)
{
#ifdef CONFIG_KERNEL_MCS
    if (size_bits < seL4_MinSchedContextBits) {
        ZF_LOGE("Invalid size bits for sc");
        return -1;
    }
    return vka_alloc_object(vka, seL4_SchedContextObject, size_bits, result);
#else
    ZF_LOGW("Allocating sched context on non RT kernel");
    return ENOSYS;
#endif
}

static inline int vka_alloc_endpoint(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, seL4_EndpointObject, seL4_EndpointBits, result);
}

static inline int vka_alloc_notification(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, seL4_NotificationObject, seL4_NotificationBits, result);
}

/* @deprecated use vka_alloc_notification */
static inline int DEPRECATED("Use vka_alloc_notification")
vka_alloc_async_endpoint(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_notification(vka, result);
}

static inline int vka_alloc_reply(UNUSED vka_t *vka, vka_object_t *result)
{
#ifdef CONFIG_KERNEL_MCS
    return vka_alloc_object(vka, seL4_ReplyObject, seL4_ReplyBits, result);
#else
    *result = (vka_object_t) {};
    ZF_LOGW("Allocating reply on non RT kernel");
    return ENOSYS;
#endif
}

static inline int vka_alloc_cnode_object(vka_t *vka, uint32_t slot_bits, vka_object_t *result)
{
    return vka_alloc_object(vka, seL4_CapTableObject, slot_bits, result);
}

/* For arch specific allocations we call upon kobject to avoid code duplication */
static inline int vka_alloc_frame(vka_t *vka, uint32_t size_bits, vka_object_t *result)
{
    return vka_alloc_object(vka, kobject_get_type(KOBJECT_FRAME, size_bits), size_bits, result);
}

/* For arch specific allocations we call upon kobject to avoid code duplication */
static inline int vka_alloc_frame_maybe_device(vka_t *vka, uint32_t size_bits, bool can_use_dev, vka_object_t *result)
{
    return vka_alloc_object_at_maybe_dev(vka, kobject_get_type(KOBJECT_FRAME, size_bits),
                                         size_bits, VKA_NO_PADDR, can_use_dev, result);
}

static inline int vka_alloc_frame_at(vka_t *vka, uint32_t size_bits, uintptr_t paddr,
                                     vka_object_t *result)
{
    return vka_alloc_object_at(vka, kobject_get_type(KOBJECT_FRAME, size_bits), size_bits,
                               paddr, result);
}

static inline int vka_alloc_page_directory(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, kobject_get_type(KOBJECT_PAGE_DIRECTORY, 0), seL4_PageDirBits, result);
}

static inline int vka_alloc_page_table(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, kobject_get_type(KOBJECT_PAGE_TABLE, 0), seL4_PageTableBits, result);
}

#ifdef CONFIG_CACHE_COLORING

static inline int vka_alloc_kernel_image(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, kobject_get_type(KOBJECT_KERNEL_IMAGE, 0), seL4_KernelImageBits, result);
}

#endif

/* Implement a kobject interface */
static inline int vka_alloc_kobject(vka_t *vka, kobject_t type, seL4_Word size_bits,
                                    vka_object_t *result)
{
    return vka_alloc_object(vka, kobject_get_type(type, size_bits), size_bits, result);
}

/* leaky versions of the object allocation functions - throws away the kobject_t */

#define LEAKY(name) \
    static inline seL4_CPtr vka_alloc_##name##_leaky(vka_t *vka) WARN_UNUSED_RESULT; \
    static inline seL4_CPtr vka_alloc_##name##_leaky(vka_t *vka) \
{\
    vka_object_t object;\
    if (vka_alloc_##name(vka, &object) != 0) {\
        return 0;\
    }\
    return object.cptr;\
}

LEAKY(tcb)
LEAKY(endpoint)
LEAKY(notification)
LEAKY(page_directory)
LEAKY(page_table)
LEAKY(sched_context)
LEAKY(reply)

static inline DEPRECATED("use vka_alloc_notification_leaky") seL4_CPtr
vka_alloc_async_endpoint_leaky(vka_t *vka)
{
    return vka_alloc_notification_leaky(vka);
}

#define LEAKY_SIZE_BITS(name) \
    static inline seL4_CPtr vka_alloc_##name##_leaky(vka_t *vka, uint32_t size_bits) WARN_UNUSED_RESULT; \
    static inline seL4_CPtr vka_alloc_##name##_leaky(vka_t *vka, uint32_t size_bits) \
{\
    vka_object_t object;\
    if (vka_alloc_##name(vka, size_bits, &object) != 0) {\
        return 0;\
    }\
    return object.cptr;\
}

LEAKY_SIZE_BITS(untyped)
LEAKY_SIZE_BITS(frame)
LEAKY_SIZE_BITS(cnode_object)

#include <vka/arch/object.h>

/*
 * Get the size (in bits) of the untyped memory required to create an object of
 * the given size.
 */
static inline unsigned long
vka_get_object_size(seL4_Word objectType, seL4_Word objectSize)
{
    switch (objectType) {
    /* Generic objects. */
    case seL4_UntypedObject:
        return objectSize;
    case seL4_TCBObject:
        return seL4_TCBBits;
#ifdef CONFIG_KERNEL_MCS
    case seL4_SchedContextObject:
        return objectSize > seL4_MinSchedContextBits ? objectSize : seL4_MinSchedContextBits;
    case seL4_ReplyObject:
        return seL4_ReplyBits;
#endif
    case seL4_EndpointObject:
        return seL4_EndpointBits;
    case seL4_NotificationObject:
        return seL4_NotificationBits;
    case seL4_CapTableObject:
        return (seL4_SlotBits + objectSize);
#ifdef CONFIG_CACHE_COLORING
    case seL4_KernelImageObject:
        return seL4_KernelImageBits;
#endif
    default:
        return vka_arch_get_object_size(objectType);
    }
}


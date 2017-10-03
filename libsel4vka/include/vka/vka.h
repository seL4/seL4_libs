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

#include <autoconf.h>

#include <sel4/sel4.h>
#include <sel4/types.h>
#include <assert.h>
#include <stdint.h>
#include <utils/util.h>
#include <vka/cspacepath_t.h>

/**
 * Allocate a slot in a cspace.
 *
 * @param data cookie for the underlying allocator
 * @param res pointer to a cptr to store the allocated slot
 * @return 0 on success
 */
typedef int (*vka_cspace_alloc_fn)(void *data, seL4_CPtr *res);

/**
 * Convert an allocated cptr to a cspacepath, for use in
 * operations such as Untyped_Retype
 *
 * @param data cookie for the underlying allocator
 * @param slot a cslot allocated by the cspace alloc function
 * @param res pointer to a cspacepath struct to fill out
 */
typedef void (*vka_cspace_make_path_fn)(void *data, seL4_CPtr slot, cspacepath_t *res);

/**
 * Free an allocated cslot
 *
 * @param data cookie for the underlying allocator
 * @param slot a cslot allocated by the cspace alloc function
 */
typedef void (*vka_cspace_free_fn)(void *data, seL4_CPtr slot);

/**
 * Allocate a portion of an untyped into an object
 *
 * @param data cookie for the underlying allocator
 * @param dest path to an empty cslot to place the cap to the allocated object
 * @param type the seL4 object type to allocate (as passed to Untyped_Retype)
 * @param size_bits the size of the object to allocate (as passed to Untyped_Retype)
 * @param res pointer to a location to store the cookie representing this allocation
 * @return 0 on success
 */
typedef int (*vka_utspace_alloc_fn)(void *data, const cspacepath_t *dest, seL4_Word type, seL4_Word size_bits, seL4_Word *res);

/**
 * Allocate a portion of an untyped into an object
 *
 * @param data cookie for the underlying allocator
 * @param dest path to an empty cslot to place the cap to the allocated object
 * @param type the seL4 object type to allocate (as passed to Untyped_Retype)
 * @param size_bits the size of the object to allocate (as passed to Untyped_Retype)
 * @param paddr The desired physical address that this object should start at
 * @param cookie pointer to a location to store the cookie representing this allocation
 * @return 0 on success
 */
typedef int (*vka_utspace_alloc_at_fn)(void *data, const cspacepath_t *dest, seL4_Word type, seL4_Word size_bits, uintptr_t paddr, seL4_Word *cookie);

/**
 * Allocate a portion of an untyped into an object
 *
 * @param data cookie for the underlying allocator
 * @param dest path to an empty cslot to place the cap to the allocated object
 * @param type the seL4 object type to allocate (as passed to Untyped_Retype)
 * @param size_bits the size of the object to allocate (as passed to Untyped_Retype)
 * @param can_use_dev whether the allocator can use device untyped instead of regular untyped
 * @param res pointer to a location to store the cookie representing this allocation
 * @return 0 on success
 */
typedef int (*vka_utspace_alloc_maybe_device_fn)(void *data, const cspacepath_t *dest, seL4_Word type, seL4_Word size_bits, bool can_use_dev, seL4_Word *res);

/**
 * Free a portion of an allocated untyped. Is the responsibility of the caller to
 * have already deleted the object (by deleting all capabilities) first
 *
 * @param data cookie for the underlying allocator
 * @param type the seL4 object type that was allocated (as passed to Untyped_Retype)
 * @param size_bits the size of the object that was allocated (as passed to Untyped_Retype)
 * @param target cookie to the allocation as given by the utspace alloc function
 */
typedef void (*vka_utspace_free_fn)(void *data, seL4_Word type, seL4_Word size_bits, seL4_Word target);

/**
 * Request the physical address of an object.
 *
 * @param data cookie for the underlying allocator
 * @param target cookie to the allocation as given by the utspace alloc function
 * @param type the seL4 object type that was allocated (as passed to Untyped_Retype)
 * @param size_bits the size of the object that was allocated (as passed to Untyped_Retype)
 * @return paddr of object, or VKA_NO_PADDR on error
 */
typedef uintptr_t (*vka_utspace_paddr_fn)(void *data, seL4_Word target, seL4_Word type, seL4_Word size_bits);

#define VKA_NO_PADDR 1

/*
 * Generic Virtual Kernel Allocator (VKA) data structure.
 *
 * This is similar in concept to the Linux VFS structures, but for
 * the seL4 kernel object allocator instead of the Linux file system.
 *
 * Alternatively, you can think of this as a abstract class in an
 * OO hierarchy, of which has several implementations.
 */

typedef struct vka {
    void *data;
    vka_cspace_alloc_fn cspace_alloc;
    vka_cspace_make_path_fn cspace_make_path;
    vka_utspace_alloc_fn utspace_alloc;
    vka_utspace_alloc_maybe_device_fn utspace_alloc_maybe_device;
    vka_utspace_alloc_at_fn utspace_alloc_at;
    vka_cspace_free_fn cspace_free;
    vka_utspace_free_fn utspace_free;
    vka_utspace_paddr_fn utspace_paddr;
} vka_t;

static inline int
vka_cspace_alloc(vka_t *vka, seL4_CPtr *res)
{
    if (!vka) {
        ZF_LOGE("vka is NULL");
        return -1;
    }

    if (!res) {
        ZF_LOGE("res is NULL");
        return -1;
    }

    if (!vka->cspace_alloc) {
        ZF_LOGE("Unimplemented");
        return -1;
    }

    return vka->cspace_alloc(vka->data, res);
}

static inline void
vka_cspace_make_path(vka_t *vka, seL4_CPtr slot, cspacepath_t *res)
{

    if (!res) {
        ZF_LOGF("res is NULL");
    }

    if (!vka) {
        ZF_LOGF("vka is NULL");
    }

    if (!vka->cspace_make_path) {
        ZF_LOGF("Unimplmented");
    }

    vka->cspace_make_path(vka->data, slot, res);
}

/*
 * Wrapper functions to make calls more convenient
 */
static inline int
vka_cspace_alloc_path(vka_t *vka, cspacepath_t *res)
{
    seL4_CPtr slot;
    int error = vka_cspace_alloc(vka, &slot);

    if (error == seL4_NoError) {
        vka_cspace_make_path(vka, slot, res);
    }

    return error;
}

static inline void
vka_cspace_free(vka_t *vka, seL4_CPtr slot)
{
#ifdef CONFIG_DEBUG_BUILD
    if (seL4_DebugCapIdentify(slot) != 0) {
        ZF_LOGF("slot is not free: call vka_cnode_delete first");
        /* this terminates the system */
    }
#endif

    if (!vka->cspace_free) {
        ZF_LOGE("Not implemented");
        return;
    }

    vka->cspace_free(vka->data, slot);
}

static inline void
vka_cspace_free_path(vka_t *vka, cspacepath_t path)
{
    vka_cspace_free(vka, path.capPtr);
}

static inline int
vka_utspace_alloc(vka_t *vka, const cspacepath_t *dest, seL4_Word type, seL4_Word size_bits, seL4_Word *res)
{
    if (!vka) {
        ZF_LOGE("vka is NULL");
        return -1;
    }

    if (!res) {
        ZF_LOGE("res is NULL");
        return -1;
    }

    if (!vka->utspace_alloc) {
        ZF_LOGE("Not implemented");
        return -1;
    }

    return vka->utspace_alloc(vka->data, dest, type, size_bits, res);
}

static inline int
vka_utspace_alloc_maybe_device(vka_t *vka, const cspacepath_t *dest, seL4_Word type,
                seL4_Word size_bits, bool can_use_dev, seL4_Word *res)
{
    if (!vka) {
        ZF_LOGE("vka is NULL");
        return -1;
    }

    if (!res) {
        ZF_LOGE("res is NULL");
        return -1;
    }

    if (!vka->utspace_alloc_maybe_device) {
        ZF_LOGE("Not implemented");
        return -1;
    }

    return vka->utspace_alloc_maybe_device(vka->data, dest, type, size_bits, can_use_dev, res);
}

static inline int
vka_utspace_alloc_at(vka_t *vka, const cspacepath_t *dest, seL4_Word type, seL4_Word size_bits,
                     uintptr_t paddr, seL4_Word *cookie)
{
    if (!vka) {
        ZF_LOGE("vka is NULL");
        return -1;
    }
    if (!cookie) {
        ZF_LOGE("cookie is NULL");
        return -1;
    }
    if (!vka->utspace_alloc_at) {
        ZF_LOGE("Not implemented");
        return -1;
    }

    return vka->utspace_alloc_at(vka->data, dest, type, size_bits, paddr, cookie);
}

static inline void
vka_utspace_free(vka_t *vka, seL4_Word type, seL4_Word size_bits, seL4_Word target)
{
    if (!vka) {
        ZF_LOGE("vka is NULL");
        return ;
    }

    if (!vka->utspace_free) {
#ifndef CONFIG_LIB_VKA_ALLOW_MEMORY_LEAKS
        ZF_LOGF("Not implemented");
        /* This terminates the system */
#endif
        return;
    }

    vka->utspace_free(vka->data, type, size_bits, target);
}

static inline uintptr_t
vka_utspace_paddr(vka_t *vka, seL4_Word target, seL4_Word type, seL4_Word size_bits)
{

    if (!vka) {
        ZF_LOGE("vka is NULL");
        return VKA_NO_PADDR;
    }

    if (!vka->utspace_paddr) {
        ZF_LOGE("Not implemented");
        return VKA_NO_PADDR;
    }

    return vka->utspace_paddr(vka->data, target, type, size_bits);
}


/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _INTERFACE_VKA_H_
#define _INTERFACE_VKA_H_

#include <autoconf.h>

#include <sel4/sel4.h>
#include <sel4/types.h>
#include <assert.h>
#include <stdint.h>
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
typedef int (*vka_utspace_alloc_fn)(void *data, const cspacepath_t *dest, seL4_Word type, seL4_Word size_bits, uint32_t *res);

/**
 * Free a portion of an allocated untyped. Is the responsibility of the caller to
 * have already deleted the object (by deleting all capabilities) first
 *
 * @param data cookie for the underlying allocator
 * @param type the seL4 object type that was allocated (as passed to Untyped_Retype)
 * @param size_bits the size of the object that was allocated (as passed to Untyped_Retype)
 * @param target cookie to the allocation as given by the utspace alloc function
 */
typedef void (*vka_utspace_free_fn)(void *data, seL4_Word type, seL4_Word size_bits, uint32_t target);

/**
 * Request the physical address of an object.
 *
 * @param data cookie for the underlying allocator
 * @param target cookie to the allocation as given by the utspace alloc function
 * @param type the seL4 object type that was allocated (as passed to Untyped_Retype)
 * @param size_bits the size of the object that was allocated (as passed to Untyped_Retype)
 * @return paddr of object, or NULL on error
 */
typedef uintptr_t (*vka_utspace_paddr_fn)(void *data, uint32_t target, seL4_Word type, seL4_Word size_bits);

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
    vka_cspace_free_fn cspace_free;
    vka_utspace_free_fn utspace_free;
    vka_utspace_paddr_fn utspace_paddr;
} vka_t;

/*
 * Dummy implementations of all different allocation functions that
 * fail if they are called
 */
static inline int dummy_vka_cspace_alloc(void *self __attribute__((unused)),
        seL4_CPtr *res __attribute__((unused)))
{
    assert(!"Not implemented");
    return -1;
}

static inline void dummy_vka_cspace_make_path(void *self __attribute__((unused)),
        seL4_CPtr slot __attribute__((unused)),
        cspacepath_t *res __attribute__((unused)))
{
    assert(!"Not implemented");
}

static inline void dummy_vka_cspace_free(void *self __attribute__((unused)),
        seL4_CPtr slot __attribute__((unused)))
{
#ifndef CONFIG_LIB_VKA_ALLOW_MEMORY_LEAKS
    assert(!"Not implemented");
#endif
}

static inline int dummy_vka_utspace_alloc(void *self __attribute__((unused)),
        const cspacepath_t *dest __attribute__((unused)),
        seL4_Word type __attribute__((unused)),
        seL4_Word size_bits __attribute__((unused)),
        uint32_t *res __attribute__((unused)))
{
    assert(!"Not implemented");
    return -1;
}

static inline void dummy_vka_utspace_free(void *self __attribute__((unused)),
        seL4_Word type __attribute__((unused)),
        seL4_Word size_bits __attribute__((unused)),
        uint32_t target __attribute__((unused)))
{
#ifndef CONFIG_LIB_VKA_ALLOW_MEMORY_LEAKS
    assert(!"Not implemented");
#endif
}

static inline uintptr_t dummy_vka_utspace_paddr(void *self __attribute__((unused)),
        uint32_t target __attribute__((unused)),
        seL4_Word type __attribute__((unused)),
        seL4_Word size_bits __attribute__((unused)))
{
    assert(!"Not implemented");
    return 0;
}

static inline int vka_cspace_alloc(vka_t *vka, seL4_CPtr *res)
{
    assert(vka);
    assert(res);
    return vka->cspace_alloc(vka->data, res);
}

static inline void vka_cspace_make_path(vka_t *vka, seL4_CPtr slot, cspacepath_t *res)
{
    assert(vka);
    assert(res);
    vka->cspace_make_path(vka->data, slot, res);
}

/*
 * Wrapper functions to make calls more convenient
 */
static inline int vka_cspace_alloc_path(vka_t *vka, cspacepath_t *res)
{
    assert(vka);
    assert(res);

    seL4_CPtr slot;
    int error = vka->cspace_alloc(vka->data, &slot);

    if (error == seL4_NoError) {
        vka_cspace_make_path(vka, slot, res);
    }

    return error;
}


static inline void vka_cspace_free(vka_t *vka, seL4_CPtr slot)
{
    assert(vka);
#ifdef CONFIG_DEBUG_BUILD
    assert(seL4_DebugCapIdentify(slot) == 0);
#endif
    vka->cspace_free(vka->data, slot);
}

static inline int vka_utspace_alloc(vka_t *vka, const cspacepath_t *dest, seL4_Word type, seL4_Word size_bits, uint32_t *res)
{
    assert(vka);
    assert(res);
    return vka->utspace_alloc(vka->data, dest, type, size_bits, res);
}

static inline void vka_utspace_free(vka_t *vka, seL4_Word type, seL4_Word size_bits, uint32_t target)
{
    assert(vka);
    vka->utspace_free(vka->data, type, size_bits, target);
}

static inline uintptr_t vka_utspace_paddr(vka_t *vka, uint32_t target, seL4_Word type, seL4_Word size_bits)
{
    assert(vka);
    assert(vka->utspace_paddr);
    return vka->utspace_paddr(vka->data, target, type, size_bits);
}

#endif /* _INTERFACE_VKA_H_ */

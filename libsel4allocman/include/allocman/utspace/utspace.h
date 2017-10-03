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

#include <stdbool.h>
#include <sel4/types.h>
#include <allocman/properties.h>
#include <allocman/cspace/cspace.h>
#include <vka/vka.h>
#include <vka/object.h>

/*
 * Marks an untyped as being usable for creating arbitrary kernel objects. Objects
 * created from such untypeds have no restrictions and can be used for anything
 */
#define ALLOCMAN_UT_KERNEL 0
/*
 * Marks an untyped as being a device region. Device regions will never be used
 * for an allocation unless explicitly requested by physical address
 */
#define ALLOCMAN_UT_DEV 1
/*
 * Marks an untyped as being from a device region, but also usable RAM. Objects can
 * be created from these untypeds if the 'canBeDev' parameter in 'alloc' is set
 * to true. An object created from one of these untypeds may have restrictions
 * in what it can be used for and may not be zeroed, as it cannot be written by
 * the kernel.
 */
#define ALLOCMAN_UT_DEV_MEM 2

/* Use the value of 1 internally to indicate the absence of a physical address.
 * This is chosen because the zero frame might actually be valid physical memory,
 * and 1 is not a valid alignment of any seL4 object, so it should never be
 * a valid physical address you can request */
#define ALLOCMAN_NO_PADDR 1

/* Convert from size of an untyped object in bytes, to the size as acceptable to a call to
 * untyped retype */
static inline size_t get_sel4_object_size(seL4_Word type, size_t size_bits) {
    if (type == seL4_CapTableObject) {
        return size_bits - seL4_SlotBits;
    } else {
        return vka_get_object_size(type, size_bits);
    }
}

struct allocman;

typedef struct utspace_interface {
    /* size_bits is always the size in memory of allocated object. This differs to the untypedretype
       semantics of size_bits when cnodes are involved */
    seL4_Word (*alloc)(struct allocman *alloc, void *utspace, size_t size_bits, seL4_Word object_type, const cspacepath_t *slot, uintptr_t paddr, bool canBeDevice, int *error);
    void (*free)(struct allocman *alloc, void *utspace, seL4_Word cookie, size_t size_bits);
    int (*add_uts)(struct allocman *alloc, void *utspace, size_t num, const cspacepath_t *uts, size_t *size_bits, uintptr_t *paddr, int utType);
    uintptr_t (*paddr)(void *utspace, seL4_Word cookie, size_t size_bits);
    struct allocman_properties properties;
    void *utspace;
}utspace_interface_t;


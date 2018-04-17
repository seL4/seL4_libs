/*
 * Copyright 2018, Data61
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

#include <errno.h>
#include <sel4/sel4.h>
#include <sel4/sel4_arch/mapping.h>
#include <utils/util.h>

typedef seL4_Error (*vspace_map_fn_t)(seL4_CPtr cap, seL4_CPtr vspace_root, seL4_Word vaddr,
                                    seL4_Word attr);

/* Map object: defines a size, type and function for mapping an architecture specific
 * page table */
typedef struct {
    /* type to pass to untype retype to create this object */
    seL4_Word type;
    /* size_bits of this object */
    seL4_Word size_bits;
    /* function that can be used to map this object */
    vspace_map_fn_t map_fn;
} vspace_map_obj_t;

/*
 * Populate a map object for a specific number of failed bits.
 *
 * @param failed_bits: the failed_bits returned by seL4_MappingFailedLookupLevel() after a failed
 *                     frame mapping operation.
 * @param obj:         object to populate with details.
 * @return 0 on success, EINVAL if failed_bits is invalid.
 */
int vspace_get_map_obj(seL4_Word failed_bits, vspace_map_obj_t *obj);

static inline seL4_Error vspace_map_obj(vspace_map_obj_t *obj, seL4_CPtr cap,
                                        seL4_CPtr vspace, seL4_Word vaddr, seL4_Word attr)
{
    ZF_LOGF_IF(obj == NULL, "obj must not be NULL");
    ZF_LOGF_IF(obj->map_fn == NULL, "obj must be populated");
    return obj->map_fn(cap, vspace, vaddr, attr);
}

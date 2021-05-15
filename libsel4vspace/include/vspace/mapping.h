/*
 * Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <errno.h>
#include <sel4/sel4.h>
#include <sel4/sel4_arch/mapping.h>
#include <utils/util.h>
#include <vspace/arch/page.h>

typedef seL4_Error(*vspace_map_fn_t)(seL4_CPtr cap, seL4_CPtr vspace_root, seL4_Word vaddr,
                                     seL4_Word attr);
typedef seL4_Error(*vspace_map_page_fn_t)(seL4_CPtr cap, seL4_CPtr vspace_root, seL4_Word vaddr,
                                          seL4_CapRights_t rights, seL4_Word attr);

static inline seL4_Error vspace_iospace_map_page(seL4_CPtr cap, seL4_CPtr root, seL4_Word vaddr,
                                                 seL4_CapRights_t rights, UNUSED seL4_Word attr)
{
#if defined(CONFIG_IOMMU) || defined(CONFIG_TK1_SMMU)
    return seL4_ARCH_Page_MapIO(cap, root, rights, vaddr);
#else
    return -1;
#endif
}

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

typedef int (*vspace_get_map_obj_fn)(seL4_Word failed_bits, vspace_map_obj_t *obj);

/*
 * Populate a map object for a specific number of failed bits.
 *
 * @param failed_bits: the failed_bits returned by seL4_MappingFailedLookupLevel() after a failed
 *                     frame mapping operation.
 * @param obj:         object to populate with details.
 * @return 0 on success, EINVAL if failed_bits is invalid.
 */
int vspace_get_map_obj(seL4_Word failed_bits, vspace_map_obj_t *obj);
/* As per vspace_get_map_obj but returns operations and sizes for an iospace (IOMMU). */
int vspace_get_iospace_map_obj(UNUSED seL4_Word failed_bits, vspace_map_obj_t *obj);
/* As per vspace_get_map_obj but returns operations and sizes for virtual page tables */
int vspace_get_ept_map_obj(seL4_Word failed_bits, vspace_map_obj_t *obj);

static inline seL4_Error vspace_map_obj(vspace_map_obj_t *obj, seL4_CPtr cap,
                                        seL4_CPtr vspace, seL4_Word vaddr, seL4_Word attr)
{
    ZF_LOGF_IF(obj == NULL, "obj must not be NULL");
    ZF_LOGF_IF(obj->map_fn == NULL, "obj must be populated");
    return obj->map_fn(cap, vspace, vaddr, attr);
}

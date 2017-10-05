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

#include <vka/vka.h>
#include <vka/capops.h>
#include <vka/object.h>

/**
 * Mint an object cap across cspaces, allocating a new slot in the dest cspace.
 *
 * @param[in] src_vka the vka the object cap was allocated with.
 * @param[in] dest_vka the vka to allocate the new slot in and mint the cap to.
 *
 * Other parameters refer to @see vka_mint_object.
 *
 */
static inline int
vka_mint_object_inter_cspace(vka_t *src_vka, vka_object_t *object, vka_t *dest_vka,  cspacepath_t *result, seL4_CapRights_t rights, seL4_Word badge)
{
    int error = vka_cspace_alloc_path(dest_vka, result);
    if (error != seL4_NoError) {
        return error;
    }

    cspacepath_t src;
    vka_cspace_make_path(src_vka, object->cptr, &src);
    return vka_cnode_mint(result, &src, rights, badge);
}

/*
 * Mint an object cap into a new cslot in the same cspace.
 *
 * @param[in] vka allocator for the cspace.
 * @param[in] object target object for cap minting.
 * @param[out] allocated cspacepath.
 * @param[in] rights the rights for the minted cap.
 * @param[in] badge the badge for the minted cap.
 */
static inline int
vka_mint_object(vka_t *vka, vka_object_t *object, cspacepath_t *result,
                seL4_CapRights_t rights, seL4_Word badge)
{
    return vka_mint_object_inter_cspace(vka, object, vka, result, rights, badge);
}


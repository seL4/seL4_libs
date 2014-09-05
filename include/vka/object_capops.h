#ifndef __VKA_OBJECT_CAPOPS_H
#define __VKA_OBJECT_CAPOPS_H

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
vka_mint_object_inter_cspace(vka_t *src_vka, vka_object_t *object, vka_t *dest_vka,  cspacepath_t *result, seL4_CapRights rights, seL4_CapData_t badge)
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
        seL4_CapRights rights, seL4_CapData_t badge) {
    return vka_mint_object_inter_cspace(vka, object, vka, result, rights, badge);
}

#endif /* __VKA_OBJECT_CAPOPS_H */

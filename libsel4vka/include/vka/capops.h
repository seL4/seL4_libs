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

/* This file provides convenient wrappers around seL4 invocations (on CNodes and Untypeds)
 * such that allocator operations are allocator independent.
 *
 * Use these and you will never have
 * to look at the cspacepath_t definition again!
 */
#include <autoconf.h>
#include <vka/cspacepath_t.h>
#include <vka/object.h>

#ifndef CONFIG_KERNEL_RT
inline static int
vka_cnode_saveCaller(const cspacepath_t* src)
{
    return seL4_CNode_SaveCaller(
               /* _service */      src->root,
               /* index */         src->capPtr,
               /* depth */         src->capDepth
           );
}
#endif

inline static int
vka_cnode_copy(const cspacepath_t* dest, const cspacepath_t* src, seL4_CapRights_t rights)
{
    return seL4_CNode_Copy(
               /* _service */      dest->root,
               /* dest_index */    dest->capPtr,
               /* destDepth */    dest->capDepth,
               /* src_root */      src->root,
               /* src_index */     src->capPtr,
               /* src_depth */     src->capDepth,
               /* rights */        rights
           );
}

inline static int
vka_cnode_delete(const cspacepath_t* src)
{
    return seL4_CNode_Delete(
               /* _service */      src->root,
               /* index */         src->capPtr,
               /* depth */         src->capDepth
           );
}

inline static int
vka_cnode_mint(const cspacepath_t* dest, const cspacepath_t* src,
               seL4_CapRights_t rights, seL4_Word badge)
{
    return seL4_CNode_Mint(
               /* _service */      dest->root,
               /* dest_index */    dest->capPtr,
               /* destDepth */    dest->capDepth,
               /* src_root */      src->root,
               /* src_index */     src->capPtr,
               /* src_depth */     src->capDepth,
               /* rights */        rights,
               /* badge  */        badge
           );
}

inline static int
vka_cnode_move(const cspacepath_t* dest, const cspacepath_t* src)
{
    return seL4_CNode_Move(
               /* _service */      dest->root,
               /* dest_index */    dest->capPtr,
               /* destDepth */    dest->capDepth,
               /* src_root */      src->root,
               /* src_index */     src->capPtr,
               /* src_depth */     src->capDepth
           );
}

inline static int
vka_cnode_mutate(const cspacepath_t* dest, const cspacepath_t* src,
                 seL4_Word badge)
{
    return seL4_CNode_Mutate(
               /* _service */      dest->root,
               /* dest_index */    dest->capPtr,
               /* destDepth */    dest->capDepth,
               /* src_root */      src->root,
               /* src_index */     src->capPtr,
               /* src_depth */     src->capDepth,
               /* badge  */        badge
           );
}

inline static int
vka_cnode_cancelBadgedSends(const cspacepath_t* src)
{
    return seL4_CNode_CancelBadgedSends(
               /* _service */      src->root,
               /* index */         src->capPtr,
               /* depth */         src->capDepth
           );
}

inline static int
vka_cnode_revoke(const cspacepath_t* src)
{
    return seL4_CNode_Revoke(
               /* _service */      src->root,
               /* index */         src->capPtr,
               /* depth */         src->capDepth
           );
}

inline static int
vka_cnode_rotate(const cspacepath_t *dest, seL4_Word dest_badge, const cspacepath_t *pivot,
                 seL4_Word pivot_badge, const cspacepath_t *src)
{
    return seL4_CNode_Rotate(dest->root, dest->capPtr, dest->capDepth, dest_badge,
                             pivot->root, pivot->capPtr, pivot->capDepth, pivot_badge,
                             src->root, src->capPtr, src->capDepth);
}

//TODO: implement rotate

/**
 * Retype num_objects objects from untyped into type starting from destination slot dest.
 *
 * size_bits is only relevant for dynamically sized objects - untypeds + captables
 */
inline static int
vka_untyped_retype(vka_object_t *untyped, int type, int size_bits, int num_objects, const cspacepath_t *dest)
{
    size_bits = vka_get_object_size(type, size_bits);
    return seL4_Untyped_Retype(untyped->cptr, type, size_bits, dest->root, dest->dest, dest->destDepth, dest->offset, num_objects);
}


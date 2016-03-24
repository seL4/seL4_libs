/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef _SEL4UTILS_SEL4_ARCH_MAPPING_H
#define _SEL4UTILS_SEL4_ARCH_MAPPING_H

#include <sel4/sel4_arch/mapping.h>
#include <vspace/arch/page.h>

typedef long (*map_fn_t)(seL4_CPtr, seL4_CPtr, seL4_Word, seL4_X86_VMAttributes);
typedef int (*alloc_fn_t)(vka_t *, vka_object_t *);

static inline int
sel4utils_create_object_at_level(vka_t *vka, seL4_Word failed_bits, vka_object_t *objects, int *num_objects, void *vaddr, seL4_CPtr vspace_root)
{
    alloc_fn_t alloc;
    map_fn_t map;
    switch (failed_bits) {
    case SEL4_MAPPING_LOOKUP_NO_PD:
        alloc = vka_alloc_page_directory;
        map = seL4_X86_PageDirectory_Map;
        break;
    case SEL4_MAPPING_LOOKUP_NO_PDPT:
        alloc = vka_alloc_pdpt;
        map = seL4_X86_PDPT_Map;
        break;
    default:
        ZF_LOGE("Invalid lookup level %d", (int)failed_bits);
        return seL4_InvalidArgument;
    }
    int error;
    error = alloc(vka, &objects[*num_objects]);
    if (error) {
        ZF_LOGE("Failed to allocate object for level %d", (int)failed_bits);
        return error;
    }
    error = map(objects[*num_objects].cptr, vspace_root, (seL4_Word)vaddr, seL4_X86_Default_VMAttributes);
    if (error == seL4_DeleteFirst) {
        /* through creating the object we must have ended up mapping this
         * level as part of the metadata creation. Delete this and keep
         * on going */
        vka_free_object(vka, &objects[*num_objects]);
        return 0;
    }
    (*num_objects)++;
    if (error) {
        ZF_LOGE("Failed to map object for level %d", (int)failed_bits);
    }
    return error;
}

#endif /* _SEL4UTILS_SEL4_ARCH_MAPPING_H */

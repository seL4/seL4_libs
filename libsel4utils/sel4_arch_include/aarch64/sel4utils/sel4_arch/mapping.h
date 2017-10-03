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

#include <sel4/sel4_arch/mapping.h>
#include <vspace/arch/page.h>
#include <vka/vka.h>
#include <vka/object.h>

typedef seL4_Error (*map_fn_t)(seL4_CPtr, seL4_CPtr, seL4_Word, seL4_ARM_VMAttributes);
typedef int (*alloc_fn_t)(vka_t *, vka_object_t *);

static inline int
sel4utils_create_object_at_level(vka_t *vka, seL4_Word failed_bits, vka_object_t *objects, int *num_objects, void *vaddr, seL4_CPtr vspace_root)
{
    alloc_fn_t alloc;
    map_fn_t map;
    vka_object_t object;
    switch (failed_bits) {
    case SEL4_MAPPING_LOOKUP_NO_PD:
        alloc = vka_alloc_page_directory;
        map = seL4_ARM_PageDirectory_Map;
        break;
    case SEL4_MAPPING_LOOKUP_NO_PUD:
        alloc = vka_alloc_page_upper_directory;
        map = seL4_ARM_PageUpperDirectory_Map;
        break;
    default:
        ZF_LOGE("Invalid lookup level %d", (int)failed_bits);
        return seL4_InvalidArgument;
    }
    int error;
    error = alloc(vka, &object);
    if (error) {
        ZF_LOGE("Failed to allocate object for level %d", (int)failed_bits);
        return error;
    }
    error = map(object.cptr, vspace_root, (seL4_Word)vaddr, seL4_ARM_Default_VMAttributes);
    if (error == seL4_DeleteFirst) {
        /* through creating the object we must have ended up mapping this
         * level as part of the metadata creation. Delete this and keep
         * on going */
        vka_free_object(vka, &object);
        return 0;
    }
    if (error) {
        ZF_LOGE("Failed to map object for level %d", (int)failed_bits);
        vka_free_object(vka, &object);
    } else {
        objects[*num_objects] = object;
        (*num_objects)++;
    }
    return error;
}


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
#ifndef _SEL4UTILS_ARCH_MAPPING_H
#define _SEL4UTILS_ARCH_MAPPING_H

#include <vspace/page.h>
#include <vka/vka.h>
#include <vka/object.h>

static inline int
sel4utils_create_object_at_level(vka_t *vka, seL4_Word failed_bits, vka_object_t *objects, int *num_objects, void *vaddr, seL4_CPtr vspace_root)
{
    int error;
    // ensure we aren't trying to create an object at a level where there can only be frames
    if (failed_bits < SEL4_MAPPING_LOOKUP_NO_PT) {
        ZF_LOGE("Invalid lookup level %d", (int)failed_bits);
        return seL4_InvalidArgument;
    }
    // riscv only has one kind of object (page table) so all we need to do is put one of those in
    vka_object_t object;
    error = vka_alloc_page_table(vka, &object);
    if (error) {
        ZF_LOGE("Failed to allocate page table");
        return error;
    }
    error = seL4_RISCV_PageTable_Map(object.cptr, vspace_root, (seL4_Word)vaddr, seL4_RISCV_Default_VMAttributes);
    if (error == seL4_DeleteFirst) {
        /* through creating the object we must have ended up mapping this
         * level as part of the metadata creation. Delete this and keep
         * on going */
        vka_free_object(vka, &object);
        return 0;
    }
    if (error) {
        ZF_LOGE("Failed to map page table");
        vka_free_object(vka, &object);
    } else {
        objects[*num_objects] = object;
        (*num_objects)++;
    }
    return error;
}

#endif /* _SEL4UTILS_ARCH_MAPPING_H */

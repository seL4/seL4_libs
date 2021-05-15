/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <autoconf.h>
#include <sel4utils/gen_config.h>

#include <inttypes.h>
#include <sel4/sel4.h>
#include <vka/object.h>
#include <vka/capops.h>
#include <sel4utils/mapping.h>
#include <sel4utils/util.h>
#include <vspace/mapping.h>

static int map_page(vka_t *vka, vspace_map_page_fn_t map_page_fn, vspace_get_map_obj_fn map_obj_fn,
                    seL4_CPtr root, seL4_CPtr frame, void *vaddr, seL4_CapRights_t rights,
                    int cacheable, vka_object_t *objects, int *num_objects)
{
    int n;
    if (!num_objects) {
        num_objects = &n;
    }

    if (!vka || !root) {
        return EINVAL;
    }

    seL4_ARCH_VMAttributes attr = cacheable ? seL4_ARCH_Default_VMAttributes :
                                  seL4_ARCH_Uncached_VMAttributes;

    *num_objects = 0;
    int error = map_page_fn(frame, root, (seL4_Word) vaddr, rights, attr);
    while (error == seL4_FailedLookup) {
        vspace_map_obj_t obj = {0};
        error = map_obj_fn(seL4_MappingFailedLookupLevel(), &obj);
        /* we should not get an incorrect values for seL4_MappingFailedLookupLevel */
        assert(error == 0);

        vka_object_t object;
        error = vka_alloc_object(vka, obj.type, obj.size_bits, &object);
        if (objects) {
            objects[*num_objects] = object;
        }
        if (error) {
            ZF_LOGE("Mapping structure allocation failed");
            return error;
        }

        error = vspace_map_obj(&obj, object.cptr, root,
                               (seL4_Word)vaddr, seL4_ARCH_Default_VMAttributes);
        if (error == seL4_DeleteFirst) {
            /* this is the case where the allocation of the page table needed to map in
             * a page table for the meta data, so delete this one and continue */
            vka_free_object(vka, &object);
        } else {
            (*num_objects)++;
            if (error) {
                ZF_LOGE("Failed to map page table %d", error);
                return error;
            }
        }
        error = map_page_fn(frame, root, (seL4_Word) vaddr, rights, attr);
    }
    if (error != seL4_NoError) {
        ZF_LOGE("Failed to map page at address %p with cap %"PRIuPTR", error: %d", vaddr, frame, error);
    }
    return error;
}

int sel4utils_map_page(vka_t *vka, seL4_CPtr vspace_root, seL4_CPtr frame, void *vaddr,
                       seL4_CapRights_t rights, int cacheable, vka_object_t *objects, int *num_objects)
{
    return map_page(vka, seL4_ARCH_Page_Map, vspace_get_map_obj, vspace_root, frame, vaddr, rights,
                    cacheable, objects, num_objects);
}

#ifndef CONFIG_ARCH_RISCV
int sel4utils_map_iospace_page(vka_t *vka, seL4_CPtr iospace, seL4_CPtr frame, seL4_Word vaddr,
                               seL4_CapRights_t rights, int cacheable, seL4_Word size_bits,
                               vka_object_t *pts, int *num_pts)
{
    return map_page(vka, vspace_iospace_map_page, vspace_get_iospace_map_obj, iospace, frame, (void *) vaddr, rights,
                    cacheable, pts, num_pts);
}
#endif /* CONFIG_ARCH_RISCV */
#ifdef CONFIG_VTX

/*map a frame into guest os's physical address space*/
int sel4utils_map_ept_page(vka_t *vka, seL4_CPtr pd, seL4_CPtr frame, seL4_Word vaddr,
                           seL4_CapRights_t rights, int cacheable, seL4_Word size_bits,
                           vka_object_t *pagetable, vka_object_t *pagedir, vka_object_t *pdpt)
{
    vka_object_t objects[3];
    int num_objects;
    int error = map_page(vka, seL4_X86_Page_MapEPT, vspace_get_ept_map_obj, pd, frame, (void *) vaddr, rights, cacheable,
                         objects,
                         &num_objects);
    *pagetable = objects[0];
    *pagedir = objects[1];
    *pdpt = objects[2];
    return error;
}

#endif /* CONFIG_VTX */

/* Some more generic routines for helping with mapping */
void *sel4utils_dup_and_map(vka_t *vka, vspace_t *vspace, seL4_CPtr page, size_t size_bits)
{
    cspacepath_t page_path;
    cspacepath_t copy_path;
    /* First need to copy the cap */
    int error = vka_cspace_alloc_path(vka, &copy_path);
    if (error != seL4_NoError) {
        return NULL;
    }
    vka_cspace_make_path(vka, page, &page_path);
    error = vka_cnode_copy(&copy_path, &page_path, seL4_AllRights);
    if (error != seL4_NoError) {
        vka_cspace_free(vka, copy_path.capPtr);
        return NULL;
    }
    /* Now map it in */
    void *mapping = vspace_map_pages(vspace, &copy_path.capPtr, NULL,  seL4_AllRights, 1, size_bits, 1);
    if (!mapping) {
        vka_cnode_delete(&copy_path);
        vka_cspace_free(vka, copy_path.capPtr);
        return NULL;
    }
    return mapping;
}

void sel4utils_unmap_dup(vka_t *vka, vspace_t *vspace, void *mapping, size_t size_bits)
{
    /* Grab a copy of the cap */
    seL4_CPtr copy = vspace_get_cap(vspace, mapping);
    cspacepath_t copy_path;
    assert(copy);
    /* now free the mapping */
    vspace_unmap_pages(vspace, mapping, 1, size_bits, VSPACE_PRESERVE);
    /* delete and free the cap */
    vka_cspace_make_path(vka, copy, &copy_path);
    vka_cnode_delete(&copy_path);
    vka_cspace_free(vka, copy);
}

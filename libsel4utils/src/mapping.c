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

#include <autoconf.h>

#include <inttypes.h>
#include <sel4/sel4.h>
#include <vka/object.h>
#include <vka/capops.h>
#include <sel4utils/mapping.h>
#include <sel4utils/util.h>

int
sel4utils_map_page(vka_t *vka, seL4_CPtr vspace_root, seL4_CPtr frame, void *vaddr,
                   seL4_CapRights_t rights, int cacheable, vka_object_t *objects, int *num_objects)
{
    assert(vka != NULL);
    assert(vspace_root != 0);
    assert(frame != 0);
    assert(vaddr != 0);
    assert(num_objects);

    seL4_ARCH_VMAttributes attr = cacheable ? seL4_ARCH_Default_VMAttributes :
                                  seL4_ARCH_Uncached_VMAttributes;

    *num_objects = 0;
    int error = seL4_ARCH_Page_Map(frame, vspace_root, (seL4_Word) vaddr, rights, attr);
    while (error == seL4_FailedLookup) {
        seL4_Word failed_bits = seL4_MappingFailedLookupLevel();
        /* handle the common case that occurs on all architectures of
         * a page table being missing */
        if (failed_bits == SEL4_MAPPING_LOOKUP_NO_PT) {
            error = vka_alloc_page_table(vka, &objects[*num_objects]);
            if (error) {
                ZF_LOGE("Page table allocation failed");
                return error;
            }
            error = seL4_ARCH_PageTable_Map(objects[*num_objects].cptr, vspace_root, (seL4_Word)vaddr, seL4_ARCH_Default_VMAttributes);
            if (error == seL4_DeleteFirst) {
                /* this is the case where the allocation of the page table needed to map in
                 * a page table for the meta data, so delete this one and continue */
                vka_free_object(vka, &objects[*num_objects]);
            } else {
                (*num_objects)++;
                if (error) {
                    ZF_LOGE("Failed to map page table %d", error);
                    return error;
                }
            }
        } else {
            error = sel4utils_create_object_at_level(vka, failed_bits, objects, num_objects, vaddr, vspace_root);
            if (error) {
                /* log message printed out by sel4utils_create_object_at_level */
                return error;
            }
        }
        error = seL4_ARCH_Page_Map(frame, vspace_root, (seL4_Word) vaddr, rights, attr);
    }
    if (error != seL4_NoError) {
        ZF_LOGE("Failed to map page at address %p with cap %"PRIuPTR", error: %d", vaddr, frame, error);
    }
    return error;
}

#if defined(CONFIG_IOMMU) || defined(CONFIG_ARM_SMMU)
int
sel4utils_map_iospace_page(vka_t *vka, seL4_CPtr iospace, seL4_CPtr frame, seL4_Word vaddr,
                           seL4_CapRights_t rights, int cacheable, seL4_Word size_bits,
                           vka_object_t *pts, int *num_pts)
{
    int error;
    assert(vka);
    assert(iospace);
    assert(frame);
    assert(cacheable);
    assert(size_bits == seL4_PageBits);
    /* Start mapping in pages and page tables until it bloody well works */
    int num = 0;
    while ( (error = seL4_ARCH_Page_MapIO(frame, iospace, rights, vaddr)) == seL4_FailedLookup && error != seL4_NoError) {
        /* Need a page table? */
        vka_object_t pt;
        error = vka_alloc_io_page_table(vka, &pt);
        if (error) {
            return error;
        }
        if (pts) {
            pts[num] = pt;
            num++;
            assert(num_pts);
            *num_pts = num;
        }
        error = seL4_ARCH_IOPageTable_Map(pt.cptr, iospace, vaddr);
        if (error != seL4_NoError) {
            return error;
        }
    }
    return error;
}

#endif /* defined(CONFIG_IOMMU) || defined(CONFIG_ARM_SMMU) */

#ifdef CONFIG_VTX

/*map a frame into guest os's physical address space*/
int
sel4utils_map_ept_page(vka_t *vka, seL4_CPtr pd, seL4_CPtr frame, seL4_Word vaddr,
                       seL4_CapRights_t rights, int cacheable, seL4_Word size_bits,
                       vka_object_t *pagetable, vka_object_t *pagedir, vka_object_t *pdpt)
{
    int ret;

    assert(vka);
    assert(pd);
    assert(frame);
    assert(size_bits);
    assert(pagetable);
    assert(pagedir);

    seL4_ARCH_VMAttributes attr = 0;

    if (!cacheable) {
        attr = seL4_X86_CacheDisabled;
    }

    /* Try map into EPT directory first. */
    ret = seL4_X86_Page_MapEPT(frame, pd, vaddr, rights, attr);
    if (ret == seL4_NoError) {
        /* Successful! */
        return ret;
    }
    if (ret != seL4_FailedLookup) {
        ZF_LOGE("Failed to map page, error: %d", ret);
        return ret;
    }

    if (size_bits != seL4_PageBits) {
        /* Allocate a directory table for a large page entry. */
        ret = vka_alloc_ept_page_directory(vka, pagedir);
        if (ret != seL4_NoError) {
            ZF_LOGE("Allocation of EPT page directory failed, error: %d", ret);
            return ret;
        }

        /* Map in the page directory. */
        ret = seL4_X86_EPTPD_Map(pagedir->cptr, pd, vaddr, attr);

        if (ret != seL4_NoError) {
            ZF_LOGE("Failed to map EPT PD, error: %d", ret);
            return ret;
        }
    } else {
        /* Allocate an EPT page table for a 4K entry. */
        ret = vka_alloc_ept_page_table(vka, pagetable);
        if (ret != seL4_NoError) {
            ZF_LOGE("allocating ept page table failed");
            return ret;
        }

        /* Map in the EPT page table. */
        ret = seL4_X86_EPTPT_Map(pagetable->cptr, pd, vaddr, attr);

        /* Mapping could fail due to missing PD. In this case, we map the PD and try again. */
        if (ret == seL4_FailedLookup) {
            /* Allocate an EPT page directory for the page table. */
            ret = vka_alloc_ept_page_directory(vka, pagedir);
            if (ret != seL4_NoError) {
                ZF_LOGE("Allocating EPT page directory failed");
                return ret;
            }

            /* Map the EPT page directory in. */
            ret = seL4_X86_EPTPD_Map(pagedir->cptr, pd, vaddr, attr);
            if (ret == seL4_FailedLookup) {
                /* Could be a missing PDPT */
                ret = vka_alloc_ept_pdpt(vka, pdpt);
                if (ret != seL4_NoError) {
                    ZF_LOGE("allocating ept pdpt failed");
                    return ret;
                }
                ret = seL4_X86_EPTPDPT_Map(pdpt->cptr, pd, vaddr, attr);
                if (ret != seL4_NoError) {
                    ZF_LOGE("Failed to map EPT PDPT, error: %d", ret);
                    return ret;
                }
                /* Try to map the page directory in again */
                ret = seL4_X86_EPTPD_Map(pagedir->cptr, pd, vaddr, attr);
            }

            if (ret != seL4_NoError) {
                ZF_LOGE("Failed to map EPT PD, error: %d", ret);
                return ret;
            }

            /* Try to map the page table again. */
            ret = seL4_X86_EPTPT_Map(pagetable->cptr, pd, vaddr, attr);
            if (ret != seL4_NoError) {
                ZF_LOGE("Second attempt at mapping EPT PT failed, error: %d", ret);
                return ret;
            }
        } else if (ret) {
            ZF_LOGE("EPT PT failed to map, error: %d", ret);
            return ret;
        }
    }

    if (ret != seL4_NoError) {
        return ret;
    }

    /* Try to map the frame again. */
    return seL4_X86_Page_MapEPT(frame, pd, vaddr, rights, attr);
}

#endif /* CONFIG_VTX */

/* Some more generic routines for helping with mapping */
void *
sel4utils_dup_and_map(vka_t *vka, vspace_t *vspace, seL4_CPtr page, size_t size_bits)
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

void
sel4utils_unmap_dup(vka_t *vka, vspace_t *vspace, void *mapping, size_t size_bits)
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

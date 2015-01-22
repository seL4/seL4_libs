/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <autoconf.h>

#ifdef CONFIG_LIB_SEL4_VKA

#include <inttypes.h>
#include <sel4/sel4.h>
#include <vka/object.h>
#include <vka/capops.h>
#include <sel4utils/mapping.h>
#include <sel4utils/util.h>

int
sel4utils_map_page(vka_t *vka, seL4_CPtr pd, seL4_CPtr frame, void *vaddr,
                   seL4_CapRights rights, int cacheable, vka_object_t *objects, int *num_objects)
{
    assert(vka != NULL);
    assert(pd != 0);
    assert(frame != 0);
    assert(vaddr != 0);
    assert(rights != 0);
    assert(num_objects);

    seL4_ARCH_VMAttributes attr = 0;
    int num = 0;

#ifdef CONFIG_ARCH_IA32
    if (!cacheable) {
        attr = seL4_IA32_CacheDisabled;
    }
#elif CONFIG_ARCH_ARM /* CONFIG_ARCH_IA32 */
    if (cacheable) {
        attr = seL4_ARM_PageCacheable;
    }
#endif /* CONFIG_ARCH_ARM */
    int error = seL4_ARCH_Page_Map(frame, pd, (seL4_Word) vaddr, rights, attr);

#ifdef CONFIG_X86_64

page_map_retry:

    if (error == seL4_FailedLookupPDPT) {
        error = vka_alloc_page_directory_pointer_table(vka, pagetable);
        if (!error) {
            error = seL4_ARCH_PageDirectoryPointerTable_Map(pagetable->cptr, pd, (seL4_Word)vaddr,
                                                            seL4_ARCH_Default_VMAttributes);
        } else {
            LOG_ERROR("Page directory pointer table allocation failed %d", error);
        }

        if (!error) {
            error = seL4_ARCH_Page_Map(frame, pd, (seL4_Word)vaddr, rights, attr);
            if (error != seL4_NoError) {
                goto page_map_retry;
            }
        } else {
            LOG_ERROR("Page directory pointer table mapping failed %d\n", error);
        }
    }

    if (error == seL4_FailedLookupPD) {
        error = vka_alloc_page_directory(vka, pagetable);
        if (!error) {
            error = seL4_ARCH_PageDirectory_Map(pagetable->cptr, pd, (seL4_Word)vaddr,
                                                seL4_ARCH_Default_VMAttributes);
        } else {
            LOG_ERROR("Page direcotry allocation failed %d\n", error);
        }

        if (!error) {
            error = seL4_ARCH_Page_Map(frame, pd, (seL4_Word)vaddr, rights, attr);
            if (error != seL4_NoError) {
                goto page_map_retry;
            }

        } else {
            LOG_ERROR("Page directory mapping failed %d\n", error);
        }
    }

#endif

    if (error == seL4_FailedLookup) {
        /* need a page table, allocate one */
        assert(objects != NULL);
        assert(*num_objects > 0);
        error = vka_alloc_page_table(vka, &objects[0]);

        /* map in the page table */
        if (!error) {
            error = seL4_ARCH_PageTable_Map(objects[0].cptr, pd, (seL4_Word) vaddr,
                                            seL4_ARCH_Default_VMAttributes);
        } else {
            LOG_ERROR("Page table allocation failed, %d", error);
        }

        if (error == seL4_DeleteFirst) {
            /* It's possible that in allocated the page table, we needed to allocate/map
             * in some memory, which caused a page table to get mapped in at the
             * same location we are wanting one. If this has happened then we can just
             * delete this page table and try the frame mapping again */
            vka_free_object(vka, &objects[0]);
            error = seL4_NoError;
        } else {
            num = 1;
        }
#ifdef CONFIG_PAE_PAGING
        if (error == seL4_FailedLookup) {
            /* need a page directory, allocate one */
            assert(*num_objects > 1);
            error = vka_alloc_page_directory(vka, &objects[1]);
            if (!error) {
                error = seL4_IA32_PageDirectory_Map(objects[1].cptr, pd, (seL4_Word) vaddr,
                                                    seL4_ARCH_Default_VMAttributes);
            } else {
                LOG_ERROR("Page directory allocation failed, %d", error);
            }
            if (error == seL4_DeleteFirst) {
                vka_free_object(vka, &objects[1]);
                error = seL4_NoError;
            } else {
                num = 2;
            }
            if (!error) {
                error = seL4_ARCH_PageTable_Map(objects[0].cptr, pd, (seL4_Word) vaddr,
                                                seL4_ARCH_Default_VMAttributes);
            } else {
                LOG_ERROR("Page directory mapping failed, %d", error);
            }
        }
#endif
        /* now try mapping the frame in again if nothing else went wrong */
        if (!error) {
            error = seL4_ARCH_Page_Map(frame, pd, (seL4_Word) vaddr, rights, attr);
        } else {
            LOG_ERROR("Page table mapping failed, %d", error);
        }
    }

    if (error != seL4_NoError) {
        LOG_ERROR("Failed to map page at address %p with cap %"PRIuPTR", error: %d", vaddr, frame, error);
    }
    *num_objects = num;

    return error;
}

#ifdef CONFIG_ARCH_IA32

#ifdef CONFIG_IOMMU
int
sel4utils_map_iospace_page(vka_t *vka, seL4_CPtr iospace, seL4_CPtr frame, seL4_Word vaddr,
                           seL4_CapRights rights, int cacheable, seL4_Word size_bits,
                           vka_object_t *pts, int *num_pts)
{
    int error;
    int num;
    assert(vka);
    assert(iospace);
    assert(frame);
    assert(cacheable);
    assert(size_bits == seL4_PageBits);
    /* Start mapping in pages and page tables until it bloody well works */
    num = 0;
    while ( (error = seL4_IA32_Page_MapIO(frame, iospace, rights, vaddr)) == seL4_FailedLookup && error != seL4_NoError) {
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
        error = seL4_IA32_IOPageTable_Map(pt.cptr, iospace, vaddr);
        if (error != seL4_NoError) {
            return error;
        }
    }
    return error;
}

#endif

#ifdef CONFIG_VTX

/*map a frame into guest os's physical address space*/
int
sel4utils_map_ept_page(vka_t *vka, seL4_CPtr pd, seL4_CPtr frame, seL4_Word vaddr,
                       seL4_CapRights rights, int cacheable, seL4_Word size_bits,
                       vka_object_t *pagetable, vka_object_t *pagedir)
{
    int ret;

    assert(vka);
    assert(pd);
    assert(frame);
    assert(rights);
    assert(size_bits);
    assert(pagetable);
    assert(pagedir);

    seL4_ARCH_VMAttributes attr = 0;

    if (!cacheable) {
        attr = seL4_IA32_CacheDisabled;
    }

    /* Try map into EPT directory first. */
    ret = seL4_IA32_Page_Map(frame, pd, vaddr, rights, attr);
    if (ret == seL4_NoError) {
        /* Successful! */
        return ret;
    }
    if (ret != seL4_FailedLookup) {
        LOG_ERROR("Failed to map page, error: %d", ret);
        return ret;
    }

    if (size_bits != seL4_PageBits) {
        /* Allocate a directory table for a large page entry. */
        ret = vka_alloc_ept_page_directory(vka, pagedir);
        if (ret != seL4_NoError) {
            LOG_ERROR("Allocation of EPT page directory failed, error: %d", ret);
            return ret;
        }

        /* Map in the page directory. */
        ret = seL4_IA32_EPTPageDirectory_Map(pagedir->cptr, pd, vaddr, attr);
        if (ret != seL4_NoError) {
            LOG_ERROR("Failed to map EPT PD, error: %d", ret);
            return ret;
        }
    } else {
        /* Allocate an EPT page table for a 4K entry. */
        ret = vka_alloc_ept_page_table(vka, pagetable);
        if (ret != seL4_NoError) {
            LOG_ERROR("allocating ept page table failed\n");
            return ret;
        }

        /* Map in the EPT page table. */
        ret = seL4_IA32_EPTPageTable_Map(pagetable->cptr, pd, vaddr, attr);

        /* Mapping could fail due to missing PD. In this case, we map the PD and try again. */
        if (ret == seL4_FailedLookup) {
            /* Allocate an EPT page directory for the page table. */
            ret = vka_alloc_ept_page_directory(vka, pagedir);
            if (ret != seL4_NoError) {
                LOG_ERROR("Allocating EPT page directory failed\n");
                return ret;
            }

            /* Map the EPT page directory in. */
            ret = seL4_IA32_EPTPageDirectory_Map(pagedir->cptr, pd, vaddr, attr);
            if (ret != seL4_NoError) {
                LOG_ERROR("Failed to map EPT PD, error: %d", ret);
                return ret;
            }

            /* Try to map the page table again. */
            ret = seL4_IA32_EPTPageTable_Map(pagetable->cptr, pd, vaddr, attr);
            if (ret != seL4_NoError) {
                LOG_ERROR("Second attempt at mapping EPT PT failed, error: %d", ret);
                return ret;
            }
        } else if (ret) {
            LOG_ERROR("EPT PT failed to map, error: %d", ret);
            return ret;
        }
    }

    if (ret != seL4_NoError) {
        return ret;
    }

    /* Try to map the frame again. */
    return seL4_IA32_Page_Map(frame, pd, vaddr, rights, attr);
}


#endif /* CONFIG_VTX */
#endif

#ifdef CONFIG_LIB_SEL4_VSPACE

/* Some more generic routines for helping with mapping */
void *
sel4utils_dup_and_map(vka_t *vka, vspace_t *vspace, seL4_CPtr page, size_t size_bits)
{
    int error;
    cspacepath_t page_path;
    cspacepath_t copy_path;
    void *mapping;
    /* First need to copy the cap */
    error = vka_cspace_alloc_path(vka, &copy_path);
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
    mapping = vspace_map_pages(vspace, &copy_path.capPtr, NULL,  seL4_AllRights, 1, size_bits, 1);
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
    /* Grap a copy of the cap */
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

#endif /* CONFIG_LIB_SEL4_VSAPCE */
#endif /* CONFIG_LIB_SEL4_VKA */

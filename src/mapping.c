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

#include <sel4/sel4.h>
#include <vka/object.h>
#include <sel4utils/mapping.h>
#include <sel4utils/util.h>

#ifndef DFMT
#ifdef CONFIG_X86_64
#define DFMT    "%lld"
#else
#define DFMT    "%d"
#endif
#endif

int
sel4utils_map_page(vka_t *vka, seL4_CPtr pd, seL4_CPtr frame, void *vaddr,
                   seL4_CapRights rights, int cacheable, vka_object_t *pagetable)
{
    assert(vka != NULL);
    assert(pd != 0);
    assert(frame != 0);
    assert(vaddr != 0);
    assert(rights != 0);
    assert(pagetable != NULL);

    seL4_ARCH_VMAttributes attr = 0;

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
        error = vka_alloc_page_table(vka, pagetable);

        /* map in the page table */
        if (!error) {
            error = seL4_ARCH_PageTable_Map(pagetable->cptr, pd, (seL4_Word) vaddr,
                    seL4_ARCH_Default_VMAttributes);
        } else {
            LOG_ERROR("Page table allocation failed, %d", error);
        }

        /* now map in the frame again, if pagetable allocation was successful */
        if (!error) {
            error = seL4_ARCH_Page_Map(frame, pd, (seL4_Word) vaddr, rights, attr);
        } else {
            LOG_ERROR("Page table mapping failed, %d", error);
        }
    }

    if (error != seL4_NoError) {
        LOG_ERROR("Failed to map page at address %p with cap "DFMT", error: %d", vaddr, frame, error);
    }

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

    if (size_bits == seL4_4MBits) {
        /* Allocate a directory table for a 4M entry. */
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


#endif
#endif
#endif /* CONFIG_LIB_SEL4_VKA */

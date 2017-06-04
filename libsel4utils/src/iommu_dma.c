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

#if defined(CONFIG_IOMMU)

#include <sel4utils/iommu_dma.h>
#include <sel4utils/vspace.h>
#include <sel4utils/vspace_internal.h>
#include <stdlib.h>
#include <vka/capops.h>
#include <string.h>
#include <utils/zf_log.h>

typedef struct dma_man {
    vka_t vka;
    vspace_t vspace;
    int num_iospaces;
    vspace_t *iospaces;
    sel4utils_alloc_data_t *iospace_data;
} dma_man_t;

static void unmap_range(dma_man_t *dma, uintptr_t addr, size_t size)
{
    uintptr_t start = ROUND_DOWN(addr, PAGE_SIZE_4K);
    uintptr_t end = addr + size;
    for (uintptr_t addr = start; addr < end; addr += PAGE_SIZE_4K) {
        for (int i = 0; i < dma->num_iospaces; i++) {
            uintptr_t *cookie = (uintptr_t*)vspace_get_cookie(dma->iospaces + i, (void*)addr);
            assert(cookie);
            (*cookie)--;
            if (*cookie == 0) {
                seL4_CPtr page = vspace_get_cap(dma->iospaces + i, (void*)addr);
                cspacepath_t page_path;
                assert(page);
                vspace_unmap_pages(dma->iospaces + i, (void*)addr, 1, seL4_PageBits, NULL);
                vka_cspace_make_path(&dma->vka, page, &page_path);
                vka_cnode_delete(&page_path);
                vka_cspace_free(&dma->vka, page);
                free(cookie);
            }
        }
    }
}

int sel4utils_iommu_dma_alloc_iospace(void* cookie, void *vaddr, size_t size)
{
    dma_man_t *dma = (dma_man_t*)cookie;
    int error;

    /* for each page duplicate and map it into all the iospaces */
    uintptr_t start = ROUND_DOWN((uintptr_t)vaddr, PAGE_SIZE_4K);
    uintptr_t end = (uintptr_t)vaddr + size;
    seL4_CPtr last_page = 0;
    for (uintptr_t addr = start; addr < end; addr += PAGE_SIZE_4K) {
        cspacepath_t page_path;
        seL4_CPtr page = vspace_get_cap(&dma->vspace, (void*)addr);
        if (!page) {
            ZF_LOGE("Failed to retrieve frame cap for malloc region. "
                    "Is your malloc backed by the correct vspace? "
                    "If you allocated your own buffer, does the dma manager's vspace "
                    "know about the caps to the frames that back the buffer?");
            unmap_range(dma, start, addr + 1);
            return -1;
        }
        if (page == last_page) {
            ZF_LOGE("Found the same frame two pages in a row. We only support 4K mappings");
            unmap_range(dma, start, addr + 1);
            return -1;
        }
        last_page = page;
        vka_cspace_make_path(&dma->vka, page, &page_path);
        /* work out the size of this page */
        for (int i = 0; i < dma->num_iospaces; i++) {
            /* see if its already mapped */
            uintptr_t *cookie = (uintptr_t*)vspace_get_cookie(dma->iospaces + i, (void*)addr);
            if (cookie) {
                /* increment the counter */
                (*cookie)++;
            } else {
                cspacepath_t copy_path;
                /* allocate slot for the cap */
                error = vka_cspace_alloc_path(&dma->vka, &copy_path);
                if (error) {
                    ZF_LOGE("Failed to allocate slot");
                    unmap_range(dma, start, addr + 1);
                    return -1;
                }
                /* copy the cap */
                error = vka_cnode_copy(&copy_path, &page_path, seL4_AllRights);
                if (error) {
                    ZF_LOGE("Failed to copy frame cap");
                    vka_cspace_free(&dma->vka, copy_path.capPtr);
                    unmap_range(dma, start, addr + 1);
                    return -1;
                }
                /* now map it in */
                reservation_t res = vspace_reserve_range_at(dma->iospaces + i, (void*)addr, PAGE_SIZE_4K, seL4_AllRights, 1);
                if (!res.res) {
                    ZF_LOGE("Failed to create a reservation");
                    vka_cnode_delete(&copy_path);
                    vka_cspace_free(&dma->vka, copy_path.capPtr);
                    unmap_range(dma, start, addr + 1);
                    return -1;
                }
                cookie = malloc(sizeof(*cookie));
                if (!cookie) {
                    ZF_LOGE("Failed to malloc %zu bytes", sizeof(*cookie));
                    vspace_free_reservation(dma->iospaces + i, res);
                    vka_cnode_delete(&copy_path);
                    vka_cspace_free(&dma->vka, copy_path.capPtr);
                    unmap_range(dma, start, addr + 1);
                    return -1;
                }
                *cookie = 1;
                error = vspace_map_pages_at_vaddr(dma->iospaces + i, &copy_path.capPtr, (uintptr_t*)&cookie, (void*)addr, 1, seL4_PageBits, res);
                if (error) {
                    ZF_LOGE("Failed to map frame into iospace");
                    free(cookie);
                    vspace_free_reservation(dma->iospaces + i, res);
                    vka_cnode_delete(&copy_path);
                    vka_cspace_free(&dma->vka, copy_path.capPtr);
                    unmap_range(dma, start, addr + 1);
                    return -1;
                }
                vspace_free_reservation(dma->iospaces + i, res);
            }
        }
    }

    return 0;
}

static void* dma_alloc(void *cookie, size_t size, int align, int cached, ps_mem_flags_t flags)
{
    int error;
    if (cached || flags != PS_MEM_NORMAL) {
        /* Going to ignore flags */
        void *ret;
        error = posix_memalign(&ret, align, size);
        if (error) {
            return NULL;
        }
        error = sel4utils_iommu_dma_alloc_iospace(cookie, ret, size);
        if (error) {
            free(ret);
            return NULL;
        }
        return ret;
    } else {
        /* do not support uncached memory */
        ZF_LOGE("Only support cached normal memory");
        return NULL;
    }
}

static void dma_free(void *cookie, void *addr, size_t size)
{
    dma_man_t *dma = cookie;
    unmap_range(dma, (uintptr_t)addr, size);
    free(addr);
}

static uintptr_t dma_pin(void *cookie, void *addr, size_t size)
{
    return (uintptr_t)addr;
}

static void dma_unpin(void *cookie, void *addr, size_t size)
{
}

static void dma_cache_op(void *cookie, void *addr, size_t size, dma_cache_op_t op)
{
    /* I have no way of knowing what this function should do on an architecture
     * that is both non cache coherent with respect to DMA, and has an IOMMU.
     * When there is a working implementation of an arm IOMMU this function
     * could be implemented */
#ifdef CONFIG_ARCH_ARM
    assert(!"not implemented");
#endif
}

int sel4utils_make_iommu_dma_alloc(vka_t *vka, vspace_t *vspace, ps_dma_man_t *dma_man, unsigned int num_iospaces, seL4_CPtr *iospaces)
{
    dma_man_t *dma = calloc(1, sizeof(*dma));
    if (!dma) {
        return -1;
    }
    dma->num_iospaces = num_iospaces;
    dma->vka = *vka;
    dma->vspace = *vspace;
    dma_man->cookie = dma;
    dma_man->dma_alloc_fn = dma_alloc;
    dma_man->dma_free_fn = dma_free;
    dma_man->dma_pin_fn = dma_pin;
    dma_man->dma_unpin_fn = dma_unpin;
    dma_man->dma_cache_op_fn = dma_cache_op;

    dma->iospaces = malloc(sizeof(vspace_t) * num_iospaces);
    if (!dma->iospaces) {
        goto error;
    }
    dma->iospace_data = malloc(sizeof(sel4utils_alloc_data_t) * num_iospaces);
    if (!dma->iospace_data) {
        goto error;
    }
    for (unsigned int i = 0; i < num_iospaces; i++) {
        int err = sel4utils_get_vspace_with_map(&dma->vspace, dma->iospaces + i, dma->iospace_data + i, &dma->vka, iospaces[i], NULL, NULL, sel4utils_map_page_iommu);
        if (err) {
            for (unsigned int j = 0; j < i; j++) {
                vspace_tear_down(dma->iospaces + i, &dma->vka);
            }
            goto error;
        }
    }
    return 0;
error:
    if (dma->iospace_data) {
        free(dma->iospace_data);
    }
    if (dma->iospaces) {
        free(dma->iospaces);
    }
    if (dma) {
        free(dma);
    }
    return -1;
}

#endif /* CONFIG_IOMMU */

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

#if defined(CONFIG_LIB_SEL4_VSPACE) && defined(CONFIG_LIB_SEL4_VKA) && defined(CONFIG_LIB_PLATSUPPORT)

#include <sel4utils/page_dma.h>
#include <vspace/vspace.h>
#include <stdlib.h>
#include <vka/capops.h>
#include <vka/kobject_t.h>
#include <utils/util.h>
#include <string.h>
#include <sel4utils/arch/cache.h>

typedef struct dma_man {
    vka_t vka;
    vspace_t vspace;
} dma_man_t;


static void dma_free(void *cookie, void *addr, size_t size) {
    dma_man_t *dma = (dma_man_t*)cookie;
    vspace_unmap_pages(&dma->vspace, addr, 1, PAGE_BITS_4K, &dma->vka);
}

static uintptr_t dma_pin(void *cookie, void *addr, size_t size) {
    dma_man_t *dma = (dma_man_t*)cookie;
    uint32_t page_cookie;
    page_cookie = vspace_get_cookie(&dma->vspace, addr);
    if (!cookie) {
        return 0;
    }
    uintptr_t paddr;
    paddr = vka_utspace_paddr(&dma->vka, page_cookie, kobject_get_type(KOBJECT_FRAME, PAGE_BITS_4K), PAGE_BITS_4K);
    if (!paddr) {
        return 0;
    }
    return (uintptr_t)paddr;
}

static void* dma_alloc(void *cookie, size_t size, int align, int cached, ps_mem_flags_t flags) {
    dma_man_t *dma = (dma_man_t*)cookie;
    /* Maximum of anything we handle is 1 4K page */
    if (size > PAGE_SIZE_4K || align > PAGE_SIZE_4K) {
        return NULL;
    }
    /* Grab a reservation, this is needed to specify the cached attribute for the mapping */
    void *base;
    reservation_t res = vspace_reserve_range(&dma->vspace, PAGE_SIZE_4K, seL4_AllRights, cached, &base);
    if (!res.res) {
        LOG_ERROR("Failed to reserve page");
        return NULL;
    }
    /* Create a new page */
    int error;
    error = vspace_new_pages_at_vaddr(&dma->vspace, base, 1, PAGE_BITS_4K, res);
    /* done with the reservation regardless of how things turn out */
    vspace_free_reservation(&dma->vspace, res);
    if (error) {
        LOG_ERROR("Failed to create page");
        return NULL;
    }
    /* Try and get the physical address so we know it will work later */
    uintptr_t paddr;
    paddr = dma_pin(cookie, base, PAGE_SIZE_4K);
    if (!paddr) {
        LOG_ERROR("No physical address for DMA page");
        dma_free(cookie, base, PAGE_SIZE_4K);
        return NULL;
    }
    return base;
}

static void dma_unpin(void *cookie, void *addr, size_t size) {
}

static void dma_cache_op(void *cookie, void *addr, size_t size, dma_cache_op_t op) {
    dma_man_t *dma = (dma_man_t*)cookie;
    seL4_CPtr root = vspace_get_root(&dma->vspace);
    /* Since we know we do not allocate across page boundaries we know that a single
     * cache op will always be sufficient */
    switch(op) {
    case DMA_CACHE_OP_CLEAN:
        seL4_ARCH_PageDirectory_Clean_Data(root, (seL4_Word)addr, (seL4_Word)addr + size);
        break;
    case DMA_CACHE_OP_INVALIDATE:
        seL4_ARCH_PageDirectory_Invalidate_Data(root, (seL4_Word)addr, (seL4_Word)addr + size);
        break;
    case DMA_CACHE_OP_CLEAN_INVALIDATE:
        seL4_ARCH_PageDirectory_CleanInvalidate_Data(root, (seL4_Word)addr, (seL4_Word)addr + size);
        break;
    }
}

int sel4utils_new_page_dma_alloc(vka_t *vka, vspace_t *vspace, ps_dma_man_t *dma_man) {
    dma_man_t *dma = (dma_man_t*)malloc(sizeof(*dma));
    if (!dma) {
        return -1;
    }
    memset(dma, 0, sizeof(*dma));
    dma->vka = *vka;
    dma->vspace = *vspace;
    dma_man->cookie = dma;
    dma_man->dma_alloc_fn = dma_alloc;
    dma_man->dma_free_fn = dma_free;
    dma_man->dma_pin_fn = dma_pin;
    dma_man->dma_unpin_fn = dma_unpin;
    dma_man->dma_cache_op_fn = dma_cache_op;
    return 0;
}

#endif /* CONFIG_LIB_SEL4_VSPACE && CONFIG_LIB_SEL4_VKA && CONFIG_LIB_PLATSUPPORT && CONFIG_IOMMU */

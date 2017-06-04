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

typedef struct dma_alloc {
    void *base;
    vka_object_t ut;
    uintptr_t paddr;
} dma_alloc_t;

static void dma_free(void *cookie, void *addr, size_t size)
{
    dma_man_t *dma = cookie;
    dma_alloc_t *alloc = (dma_alloc_t*)vspace_get_cookie(&dma->vspace, addr);
    assert(alloc);
    assert(alloc->base == addr);
    int num_pages = BIT(alloc->ut.size_bits) / PAGE_SIZE_4K;
    for (int i = 0; i < num_pages; i++) {
        cspacepath_t path;
        seL4_CPtr frame = vspace_get_cap(&dma->vspace, addr + i * PAGE_SIZE_4K);
        vspace_unmap_pages(&dma->vspace, addr + i * PAGE_SIZE_4K, 1, PAGE_BITS_4K, NULL);
        vka_cspace_make_path(&dma->vka, frame, &path);
        vka_cnode_delete(&path);
        vka_cspace_free(&dma->vka, frame);
    }
    vka_free_object(&dma->vka, &alloc->ut);
    free(alloc);
}

static uintptr_t dma_pin(void *cookie, void *addr, size_t size)
{
    dma_man_t *dma = cookie;
    dma_alloc_t *alloc = (dma_alloc_t*)vspace_get_cookie(&dma->vspace, addr);
    if (!alloc) {
        return 0;
    }
    uint32_t diff = addr - alloc->base;
    return alloc->paddr + diff;
}

static void* dma_alloc(void *cookie, size_t size, int align, int cached, ps_mem_flags_t flags)
{
    dma_man_t *dma = cookie;
    cspacepath_t *frames = NULL;
    reservation_t res = {NULL};
    dma_alloc_t *alloc = NULL;
    unsigned int num_frames = 0;
    void *base = NULL;
    /* We align to the 4K boundary, but do not support more */
    if (align > PAGE_SIZE_4K) {
        return NULL;
    }
    /* Round up to the next page size */
    size = ROUND_UP(size, PAGE_SIZE_4K);
    /* Then round up to the next power of 2 size. This is because untypeds are allocated
     * in powers of 2 */
    size_t size_bits = LOG_BASE_2(size);
    if (BIT(size_bits) != size) {
        size_bits++;
    }
    size = BIT(size_bits);
    /* Allocate an untyped */
    vka_object_t ut;
    int error = vka_alloc_untyped(&dma->vka, size_bits, &ut);
    if (error) {
        ZF_LOGE("Failed to allocate untyped of size %zu", size_bits);
        return NULL;
    }
    /* Get the physical address */
    uintptr_t paddr = vka_utspace_paddr(&dma->vka, ut.ut, seL4_UntypedObject, size_bits);
    if (paddr == VKA_NO_PADDR) {
        ZF_LOGE("Allocated untyped has no physical address");
        goto handle_error;
    }
    /* Allocate all the frames */
    num_frames = size / PAGE_SIZE_4K;
    frames = calloc(num_frames, sizeof(cspacepath_t));
    if (!frames) {
        goto handle_error;
    }
    for (unsigned i = 0; i < num_frames; i++) {
        error = vka_cspace_alloc_path(&dma->vka, &frames[i]);
        if (error) {
            goto handle_error;
        }
        error = seL4_Untyped_Retype(ut.cptr, kobject_get_type(KOBJECT_FRAME, PAGE_BITS_4K), size_bits, frames[i].root, frames[i].dest, frames[i].destDepth, frames[i].offset, 1);
        if (error != seL4_NoError) {
            goto handle_error;
        }
    }
    /* Grab a reservation */
    res = vspace_reserve_range(&dma->vspace, size, seL4_AllRights, cached, &base);
    if (!res.res) {
        ZF_LOGE("Failed to reserve");
        return NULL;
    }
    alloc = malloc(sizeof(*alloc));
    if (alloc == NULL) {
        goto handle_error;
    }
    alloc->base = base;
    alloc->ut = ut;
    alloc->paddr = paddr;
    /* Map in all the pages */
    for (unsigned i = 0; i < num_frames; i++) {
        error = vspace_map_pages_at_vaddr(&dma->vspace, &frames[i].capPtr, (uintptr_t*)&alloc, base + i * PAGE_SIZE_4K, 1, PAGE_BITS_4K, res);
        if (error) {
            goto handle_error;
        }
    }
    /* no longer need the reservation */
    vspace_free_reservation(&dma->vspace, res);
    return base;
handle_error:
    if (alloc) {
        free(alloc);
    }
    if (res.res) {
        vspace_unmap_pages(&dma->vspace, base, num_frames, PAGE_BITS_4K, NULL);
        vspace_free_reservation(&dma->vspace, res);
    }
    if (frames) {
        for (int i = 0; i < num_frames; i++) {
            if (frames[i].capPtr) {
                vka_cnode_delete(&frames[i]);
                vka_cspace_free(&dma->vka, frames[i].capPtr);
            }
        }
        free(frames);
    }
    vka_free_object(&dma->vka, &ut);
    return NULL;
}

static void dma_unpin(void *cookie, void *addr, size_t size)
{
}

static void dma_cache_op(void *cookie, void *addr, size_t size, dma_cache_op_t op)
{
    dma_man_t *dma = cookie;
    seL4_CPtr root = vspace_get_root(&dma->vspace);
    uintptr_t end = (uintptr_t)addr + size;
    uintptr_t cur = (uintptr_t)addr;
    while (cur < end) {
        uintptr_t top = ROUND_UP(cur + 1, PAGE_SIZE_4K);
        if (top > end) {
            top = end;
        }
        switch (op) {
        case DMA_CACHE_OP_CLEAN:
            seL4_ARCH_PageDirectory_Clean_Data(root, (seL4_Word)cur, (seL4_Word)top);
            break;
        case DMA_CACHE_OP_INVALIDATE:
            seL4_ARCH_PageDirectory_Invalidate_Data(root, (seL4_Word)cur, (seL4_Word)top);
            break;
        case DMA_CACHE_OP_CLEAN_INVALIDATE:
            seL4_ARCH_PageDirectory_CleanInvalidate_Data(root, (seL4_Word)cur, (seL4_Word)top);
            break;
        }
        cur = top;
    }
}

int sel4utils_new_page_dma_alloc(vka_t *vka, vspace_t *vspace, ps_dma_man_t *dma_man)
{
    dma_man_t *dma = calloc(1, sizeof(*dma));
    if (!dma) {
        return -1;
    }
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

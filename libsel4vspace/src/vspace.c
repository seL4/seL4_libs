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
#include <vspace/vspace.h>
#include <utils/page.h>

void *
vspace_new_sized_stack(vspace_t *vspace, size_t n_pages)
{
    int error = 0;
    void *vaddr = NULL;

    /* one extra page for the guard */
    reservation_t reserve = vspace_reserve_range(vspace, (n_pages + 1) * PAGE_SIZE_4K,
                                                 seL4_AllRights, 1, &vaddr);

    if (reserve.res == NULL) {
        return NULL;
    }

    /* reserve the first page as the guard */
    uintptr_t stack_bottom =  (uintptr_t) vaddr + PAGE_SIZE_4K;

    /* create and map the pages */
    error = vspace_new_pages_at_vaddr(vspace, (void *) stack_bottom, n_pages, seL4_PageBits, reserve);

    if (error) {
        vspace_free_reservation(vspace, reserve);
        return NULL;
    }

    return (void *) (stack_bottom + (n_pages * PAGE_SIZE_4K));
}

void
vspace_free_sized_stack(vspace_t *vspace, void *stack_top, size_t n_pages)
{
    if (n_pages > 0) {
        uintptr_t stack_bottom = (uintptr_t) stack_top - (n_pages * PAGE_SIZE_4K);
        vspace_unmap_pages(vspace, (void *) stack_bottom, n_pages,
                           seL4_PageBits, (vka_t *) VSPACE_FREE);
        vspace_free_reservation_by_vaddr(vspace, (void *) (stack_bottom - PAGE_SIZE_4K));
    } else {
        ZF_LOGW("Freeing 0 sized stack");
    }
}

void *
vspace_new_ipc_buffer(vspace_t *vspace, seL4_CPtr *page)
{

    void *vaddr = vspace_new_pages(vspace, seL4_AllRights, 1, seL4_PageBits);
    if (vaddr == NULL) {
        return NULL;
    }

    *page = vspace_get_cap(vspace, vaddr);

    return vaddr;

}

void
vspace_free_ipc_buffer(vspace_t *vspace, void *addr)
{
    vspace_unmap_pages(vspace, addr, 1, seL4_PageBits, VSPACE_FREE);
}

void *
vspace_share_mem(vspace_t *from, vspace_t *to, void *start, int num_pages, size_t size_bits,
                 seL4_CapRights_t rights, int cacheable)
{
    void *result;

    /* reserve a range to map the shared memory in to */
    reservation_t res = vspace_reserve_range_aligned(to, num_pages * (BIT(size_bits)), size_bits,
                                                     rights, cacheable, &result);

    if (res.res == NULL) {
        ZF_LOGE("Failed to reserve range");
        return NULL;
    }

    int error = vspace_share_mem_at_vaddr(from, to, start, num_pages, size_bits, result, res);
    if (error) {
        /* fail */
        result = NULL;
    }

    /* clean up reservation */
    vspace_free_reservation(to, res);

    return result;
}

void *
vspace_new_pages_with_config(vspace_t *vspace, vspace_new_pages_config_t *config, seL4_CapRights_t rights)
{
    reservation_t res;
    if (config->vaddr == NULL) {
        res = vspace_reserve_range_aligned(vspace, config->num_pages * SIZE_BITS_TO_BYTES(config->size_bits),
                                                         config->size_bits,
                                                         rights, true, &config->vaddr);
    } else {
        res =  vspace_reserve_range_at(vspace, config->vaddr,
                                                         config->num_pages * SIZE_BITS_TO_BYTES(config->size_bits),
                                                         rights, true);
    }
    if (res.res == NULL) {
        ZF_LOGE("Failed to reserve range");
        return NULL;
    }

    UNUSED int error = vspace_new_pages_at_vaddr_with_config(vspace, config, res);
    vspace_free_reservation(vspace, res);

    if (error) {
        return NULL;
    }

    return config->vaddr;
}

/* this function is for backwards compatibility after interface change */
reservation_t
vspace_reserve_range(vspace_t *vspace, size_t bytes,
                     seL4_CapRights_t rights, int cacheable, void **vaddr)
{
    return vspace_reserve_range_aligned(vspace, bytes, seL4_PageBits, rights, cacheable, vaddr);
}

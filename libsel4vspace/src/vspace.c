
#include <autoconf.h>
#include <vspace/vspace.h>
#include <utils/page.h>

#define STACK_PAGES (BYTES_TO_4K_PAGES(CONFIG_SEL4UTILS_STACK_SIZE))

void *
vspace_new_stack(vspace_t *vspace)
{
    int error = 0;
    void *vaddr = NULL;

    /* one extra page for the guard */
    reservation_t reserve = vspace_reserve_range(vspace, (STACK_PAGES + 1) * PAGE_SIZE_4K,
                                                 seL4_AllRights, 1, &vaddr);

    if (reserve.res == NULL) {
        return NULL;
    }

    /* reserve the first page as the guard */
    uintptr_t stack_bottom =  (uintptr_t) vaddr + PAGE_SIZE_4K;

    /* create and map the pages */
    error = vspace_new_pages_at_vaddr(vspace, (void *) stack_bottom, STACK_PAGES, seL4_PageBits, reserve);

    if (error) {
        vspace_free_reservation(vspace, reserve);
        return NULL;
    }

    return (void *) (stack_bottom + CONFIG_SEL4UTILS_STACK_SIZE);
}

void
vspace_free_stack(vspace_t *vspace, void *stack_top)
{
    uintptr_t stack_bottom = (uintptr_t) stack_top - CONFIG_SEL4UTILS_STACK_SIZE;
    vspace_unmap_pages(vspace, (void *) stack_bottom, STACK_PAGES,
                       seL4_PageBits, (vka_t *) VSPACE_FREE);
    vspace_free_reservation_by_vaddr(vspace, (void *) (stack_bottom - PAGE_SIZE_4K));
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
                 seL4_CapRights rights, int cacheable)
{
    void *result;

    /* reserve a range to map the shared memory in to */
    reservation_t res = vspace_reserve_range_aligned(to, num_pages * (1 << size_bits), size_bits,
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
vspace_map_pages(vspace_t *vspace, seL4_CPtr caps[], uint32_t cookies[], seL4_CapRights rights,
                 size_t num_caps, size_t size_bits, int cacheable)
{

    void *vaddr;
    reservation_t res = vspace_reserve_range_aligned(vspace,
                                                     num_caps * SIZE_BITS_TO_BYTES(size_bits), size_bits,
                                                     rights, cacheable, &vaddr);

    if (res.res == NULL) {
        ZF_LOGE("Failed to reserve range");
        return NULL;
    }

    int error = vspace_map_pages_at_vaddr(vspace, caps, cookies, vaddr, num_caps, size_bits, res);

    vspace_free_reservation(vspace, res);

    if (error) {
        return NULL;
    }

    return vaddr;
}

void *
vspace_new_pages(vspace_t *vspace, seL4_CapRights rights, size_t num_pages, size_t size_bits)
{
    void *vaddr;
    reservation_t res = vspace_reserve_range_aligned(vspace,
                                                     num_pages * SIZE_BITS_TO_BYTES(size_bits), size_bits,
                                                     rights, true, &vaddr);

    if (res.res == NULL) {
        ZF_LOGE("Failed to reserve range");
        return NULL;
    }

    int error = vspace_new_pages_at_vaddr(vspace, vaddr, num_pages, size_bits, res);

    vspace_free_reservation(vspace, res);

    if (error) {
        return NULL;
    }

    return vaddr;
}

/* this function is for backwards compatibility after interface change */
reservation_t
vspace_reserve_range(vspace_t *vspace, size_t bytes,
                     seL4_CapRights rights, int cacheable, void **vaddr)
{
    return vspace_reserve_range_aligned(vspace, bytes, seL4_PageBits, rights, cacheable, vaddr);
}



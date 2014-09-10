
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
    void *stack_bottom =  vaddr + PAGE_SIZE_4K;

    /* create and map the pages */
    error = vspace_new_pages_at_vaddr(vspace, stack_bottom, STACK_PAGES, seL4_PageBits, reserve);

    if (error) {
        vspace_free_reservation(vspace, reserve);
        return NULL;
    }

    return stack_bottom + CONFIG_SEL4UTILS_STACK_SIZE;
}

void
vspace_free_stack(vspace_t *vspace, void *stack_top)
{
    void *stack_bottom = stack_top - CONFIG_SEL4UTILS_STACK_SIZE;
    vspace_unmap_pages(vspace, stack_bottom, STACK_PAGES,
                       seL4_PageBits, (vka_t *) VSPACE_FREE);
    vspace_free_reservation_by_vaddr(vspace, stack_bottom - PAGE_SIZE_4K);
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



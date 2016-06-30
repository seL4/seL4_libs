/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include <autoconf.h>

#include <stdlib.h>
#include <string.h>

#include <vspace/vspace.h>
#include <sel4utils/vspace.h>
#include <sel4utils/vspace_internal.h>
#include <vka/capops.h>

#include "vmm/platform/guest_vspace.h"

typedef struct guest_vspace {
    /* We abuse struct ordering and this member MUST be the first
     * thing in the struct */
    struct sel4utils_alloc_data vspace_data;
    /* additional vspace to place all mappings into. We will maintain
     * a translation between this and the guest */
    vspace_t vmm_vspace;
    /* use a vspace implementation as a sparse data structure to track
     * the translation from guest to vmm */
    struct sel4utils_alloc_data translation_vspace_data;
    vspace_t translation_vspace;
#ifdef CONFIG_IOMMU
    /* debug flag for checking if we add io spaces late */
    int done_mapping;
    int num_iospaces;
    seL4_CPtr *iospaces;
#endif
} guest_vspace_t;

static int
guest_vspace_map(vspace_t *vspace, seL4_CPtr cap, void *vaddr, seL4_CapRights rights,
        int cacheable, size_t size_bits) {
    struct sel4utils_alloc_data *data = get_alloc_data(vspace);
    int error;
    /* this type cast works because the alloc data was at the start of the struct
     * so it has the same address.
     * This conversion is guaranteed to work by the C standard */
    guest_vspace_t *guest_vspace = (guest_vspace_t*) data;
    /* perfrom the ept mapping */
    error = sel4utils_map_page_ept(vspace, cap, vaddr, rights, cacheable, size_bits);
    if (error) {
        return error;
    }
    /* duplicate the cap so we can do a mapping */
    cspacepath_t orig_path;
    vka_cspace_make_path(guest_vspace->vspace_data.vka, cap, &orig_path);
    cspacepath_t new_path;
    error = vka_cspace_alloc_path(guest_vspace->vspace_data.vka, &new_path);
    if (error) {
        ZF_LOGE("Failed to allocate cslot to duplicate frame cap");
        return error;
    }
    error = vka_cnode_copy(&new_path, &orig_path, seL4_AllRights);
    assert(error == seL4_NoError);
    /* perform the regular mapping */
    void *vmm_vaddr = vspace_map_pages(&guest_vspace->vmm_vspace, &new_path.capPtr, NULL, seL4_AllRights, 1, size_bits, cacheable);
    if (!vmm_vaddr){
        ZF_LOGE("Failed to map into VMM vspace");
        return -1;
    }
    /* add translation information. give dummy cap value of 42 as it cannot be zero
     * but we really just want to store information in the cookie */
    error = update_entries(&guest_vspace->translation_vspace, (uintptr_t)vaddr, 42, size_bits, (uint32_t)vmm_vaddr);
    if (error){
        ZF_LOGE("Failed to add translation information");
        return error;
    }
#ifdef CONFIG_IOMMU
    /* set the mapping bit */
    guest_vspace->done_mapping = 1;
    /* map into all the io spaces */
    for (int i = 0; i < guest_vspace->num_iospaces; i++) {
        error = vka_cspace_alloc_path(guest_vspace->vspace_data.vka, &new_path);
        if (error) {
            ZF_LOGE("Failed to allocate cslot to duplicate frame cap");
            return error;
        }
        error = vka_cnode_copy(&new_path, &orig_path, seL4_AllRights);
        assert(error == seL4_NoError);
        error = sel4utils_map_iospace_page(guest_vspace->vspace_data.vka, guest_vspace->iospaces[i], new_path.capPtr, (uintptr_t)vaddr, rights, cacheable, size_bits, NULL, NULL);
        if (error) {
            return error;
        }
    }
#else
    (void)guest_vspace;
#endif
    return 0;
}

#ifdef CONFIG_IOMMU
int vmm_guest_vspace_add_iospace(vspace_t *vspace, seL4_CPtr iospace) {
    struct sel4utils_alloc_data *data = get_alloc_data(vspace);
    guest_vspace_t *guest_vspace = (guest_vspace_t*) data;

    assert(!guest_vspace->done_mapping);

    guest_vspace->iospaces = realloc(guest_vspace->iospaces, sizeof(seL4_CPtr) * (guest_vspace->num_iospaces + 1));
    assert(guest_vspace->iospaces);

    guest_vspace->iospaces[guest_vspace->num_iospaces] = iospace;
    guest_vspace->num_iospaces++;
    return 0;
}
#endif

int vmm_get_guest_vspace(vspace_t *loader, vspace_t *vmm, vspace_t *new_vspace, vka_t *vka, seL4_CPtr page_directory) {
    int error;
    guest_vspace_t *vspace = malloc(sizeof(*vspace));
    if (!vspace) {
        ZF_LOGE("Malloc failed");
        return -1;
    }
#ifdef CONFIG_IOMMU
    vspace->done_mapping = 0;
    vspace->num_iospaces = 0;
    vspace->iospaces = malloc(0);
    assert(vspace->iospaces);
#endif
    vspace->vmm_vspace = *vmm;
    error = sel4utils_get_vspace(loader, &vspace->translation_vspace, &vspace->translation_vspace_data, vka, page_directory, NULL, NULL);
    if (error) {
        ZF_LOGE("Failed to create translation vspace");
        return error;
    }
    error = sel4utils_get_vspace_with_map(loader, new_vspace, &vspace->vspace_data, vka, page_directory, NULL, NULL, guest_vspace_map);
    if (error) {
        ZF_LOGE("Failed to create guest vspace");
        return error;
    }
    return 0;
}

/* Helpers for use with touch below */
int vmm_guest_get_phys_data_help(uintptr_t addr, void *vaddr, size_t size,
        size_t offset, void *cookie) {
    memcpy(cookie, vaddr, size);

    return 0;
}

int vmm_guest_set_phys_data_help(uintptr_t addr, void *vaddr, size_t size,
        size_t offset, void *cookie) {
    memcpy(vaddr, cookie, size);

    return 0;
}

int vmm_guest_vspace_touch(vspace_t *vspace, uintptr_t addr, size_t size, vmm_guest_vspace_touch_callback callback, void *cookie) {
    struct sel4utils_alloc_data *data = get_alloc_data(vspace);
    guest_vspace_t *guest_vspace = (guest_vspace_t*) data;
    uintptr_t current_addr;
    uintptr_t next_addr;
    uintptr_t end_addr = (uintptr_t)(addr + size);
    for (current_addr = (uintptr_t)addr; current_addr < end_addr; current_addr = next_addr) {
        uintptr_t current_aligned = PAGE_ALIGN_4K(current_addr);
        uintptr_t next_page_start = current_aligned + PAGE_SIZE_4K;
        next_addr = MIN(end_addr, next_page_start);
        void *vaddr = (void*)sel4utils_get_cookie(&guest_vspace->translation_vspace, (void*)current_aligned);
        if (!vaddr) {
            ZF_LOGE("Failed to get cookie at 0x%x", current_aligned);
            return -1;
        }
        int result = callback(current_addr, (void*)(vaddr + (current_addr - current_aligned)), next_addr - current_addr, current_addr - addr, cookie);
        if (result) {
            return result;
        }
    }
    return 0;
}

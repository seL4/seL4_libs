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

#include <vspace/vspace.h>
#include <sel4utils/vspace.h>
#include <sel4utils/vspace_internal.h>
#include <vka/capops.h>

#include "vmm/platform/guest_vspace.h"

typedef struct guest_vspace {
    /* We abuse struct ordering and this member MUST be the first
     * thing in the struct */
    struct sel4utils_alloc_data vspace_data;
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
#ifdef CONFIG_IOMMU
    /* set the mapping bit */
    guest_vspace->done_mapping = 1;
    /* map into all the io spaces */
    for (int i = 0; i < guest_vspace->num_iospaces; i++) {
        cspacepath_t orig_path;
        cspacepath_t new_path;
        error = vka_cspace_alloc_path(guest_vspace->vspace_data.vka, &new_path);
        if (error) {
            LOG_ERROR("Failed to allocate cslot to duplicate frame cap");
            return error;
        }
        vka_cspace_make_path(guest_vspace->vspace_data.vka, cap, &orig_path);
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

int vmm_get_guest_vspace(vspace_t *loader, vspace_t *new_vspace, vka_t *vka, seL4_CPtr page_directory) {
    int error;
    guest_vspace_t *vspace = malloc(sizeof(*vspace));
    if (!vspace) {
        LOG_ERROR("Malloc failed");
        return -1;
    }
#ifdef CONFIG_IOMMU
    vspace->done_mapping = 0;
    vspace->num_iospaces = 0;
    vspace->iospaces = malloc(0);
    assert(vspace->iospaces);
#endif
    error = sel4utils_get_vspace_with_map(loader, new_vspace, &vspace->vspace_data, vka, page_directory, NULL, NULL, guest_vspace_map);
    if (error) {
        LOG_ERROR("Failed to create guest vspace");
        return error;
    }
    return 0;
}

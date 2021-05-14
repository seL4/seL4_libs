/*
 * Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <vspace/mapping.h>

int vspace_get_map_obj(seL4_Word failed_bits, vspace_map_obj_t *obj) {
    if (unlikely(obj == NULL)) {
        return EINVAL;
    }

    switch (failed_bits) {
    case SEL4_MAPPING_LOOKUP_NO_PT:
        obj->size_bits = seL4_PageTableBits;
        obj->type = seL4_ARM_PageTableObject;
        obj->map_fn = seL4_ARM_PageTable_Map;
        return 0;
    case SEL4_MAPPING_LOOKUP_NO_PD:
        obj->size_bits = seL4_PageDirBits;
        obj->type = seL4_ARM_PageDirectoryObject;
        obj->map_fn = seL4_ARM_PageDirectory_Map;
        return 0;
#if !(defined CONFIG_ARM_HYPERVISOR_SUPPORT && defined CONFIG_ARM_PA_SIZE_BITS_40)
    case SEL4_MAPPING_LOOKUP_NO_PUD:
        obj->size_bits = seL4_PUDBits;
        obj->type = seL4_ARM_PageUpperDirectoryObject;
        obj->map_fn = seL4_ARM_PageUpperDirectory_Map;
        return 0;
#endif
    default:
        return EINVAL;
    }
}

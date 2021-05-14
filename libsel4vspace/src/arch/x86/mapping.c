/*
 * Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <autoconf.h>
#include <vspace/mapping.h>
#include <sel4/arch/mapping.h>

static seL4_Error vspace_map_io(seL4_CPtr cap, seL4_CPtr iospace_root, seL4_Word vaddr, UNUSED seL4_Word attr)
{
#ifdef CONFIG_IOMMU
    return seL4_X86_IOPageTable_Map(cap, iospace_root, vaddr);
#endif
}

int vspace_get_iospace_map_obj(UNUSED seL4_Word failed_bits, vspace_map_obj_t *obj)
{
    if (unlikely(obj == NULL) || !config_set(CONFIG_IOMMU)) {
        return EINVAL;
    }
#ifdef CONFIG_IOMMU
    obj->size_bits = seL4_IOPageTableBits;
    obj->type = seL4_X86_IOPageTableObject;
    obj->map_fn = vspace_map_io;
    return 0;
#endif
}

int vspace_get_ept_map_obj(seL4_Word failed_bits, vspace_map_obj_t *obj)
{
    switch (failed_bits) {
#ifdef CONFIG_VTX
    case SEL4_MAPPING_LOOKUP_NO_EPTPT:
        obj->size_bits = seL4_X86_EPTPTBits;
        obj->type = seL4_X86_EPTPTObject;
        obj->map_fn = seL4_X86_EPTPT_Map;
        return 0;
    case SEL4_MAPPING_LOOKUP_NO_EPTPD:
        obj->size_bits = seL4_X86_EPTPDBits;
        obj->type = seL4_X86_EPTPDObject;
        obj->map_fn = seL4_X86_EPTPD_Map;
        return 0;
    case SEL4_MAPPING_LOOKUP_NO_EPTPDPT:
        obj->size_bits = seL4_X86_EPTPDPTBits;
        obj->type = seL4_X86_EPTPDPTObject;
        obj->map_fn = seL4_X86_EPTPDPT_Map;
        return 0;
#endif
    default:
        return EINVAL;
    }
}

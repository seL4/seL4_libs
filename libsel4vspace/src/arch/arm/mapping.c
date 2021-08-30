/*
 * Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <autoconf.h>
#include <vspace/mapping.h>

#ifdef CONFIG_TK1_SMMU
static seL4_Error vspace_map_io(seL4_CPtr cap, seL4_CPtr iospace_root,
                                seL4_Word vaddr, UNUSED seL4_Word attr)
{
    return seL4_ARM_IOPageTable_Map(cap, iospace_root, vaddr);
}
#endif /* CONFIG_TK1_SMMU */

int vspace_get_iospace_map_obj(UNUSED seL4_Word failed_bits, vspace_map_obj_t *obj)
{
    if (unlikely(obj == NULL) || !config_set(CONFIG_TK1_SMMU)) {
        return EINVAL;
    }
#ifdef CONFIG_TK1_SMMU
    obj->size_bits = seL4_IOPageTableBits;
    obj->type = seL4_ARM_IOPageTableObject;
    obj->map_fn = vspace_map_io;
#endif
    return 0;
}

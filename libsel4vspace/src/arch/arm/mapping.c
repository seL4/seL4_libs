/*
 * Copyright 2018, Data61
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
#include <vspace/mapping.h>

static seL4_Error vspace_map_io(seL4_CPtr cap, seL4_CPtr iospace_root, seL4_Word vaddr, UNUSED seL4_Word attr)
{
#ifdef CONFIG_TK1_SMMU
    return seL4_ARM_IOPageTable_Map(cap, iospace_root, vaddr);
#endif
}

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

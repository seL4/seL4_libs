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

#include <vspace/mapping.h>

int vspace_get_map_obj(seL4_Word failed_bits, vspace_map_obj_t *obj) {
    if (unlikely(obj == NULL)) {
        return EINVAL;
    }

    switch (failed_bits) {
    case SEL4_MAPPING_LOOKUP_NO_PT:
        obj->size_bits = seL4_PageTableBits;
        obj->type = seL4_X86_PageTableObject;
        obj->map_fn = seL4_X86_PageTable_Map;
        return 0;
    default:
        return EINVAL;
    }
}

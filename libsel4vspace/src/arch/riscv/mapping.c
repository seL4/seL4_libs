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

    if (failed_bits < SEL4_MAPPING_LOOKUP_NO_PT) {
        ZF_LOGE("Invalid lookup level %d", (int) failed_bits);
        return EINVAL;
    }

    obj->size_bits = seL4_PageTableBits;
    obj->type = seL4_RISCV_PageTableObject;
    obj->map_fn = seL4_RISCV_PageTable_Map;
    return 0;
}

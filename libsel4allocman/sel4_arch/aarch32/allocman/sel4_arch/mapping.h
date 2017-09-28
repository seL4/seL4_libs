/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#pragma once

#include <allocman/allocman.h>

static inline seL4_Error allocman_sel4_arch_create_object_at_level(allocman_t *alloc, seL4_Word bits, cspacepath_t *path, void *vaddr, seL4_CPtr vspace_root) {
    /* ARM has no paging objects beyond the common page table
     * so this function should never get called */
    ZF_LOGF("Invalid lookup level");
    return seL4_InvalidArgument;
}


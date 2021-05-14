/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <allocman/allocman.h>
#include <allocman/cspace/cspace.h>
#include <vka/capops.h>

/* Helper routines for treating cspaces in a high level way. Giving you seL4 like operations
 * at the cspace level instead of the cap/slot level */

static inline int cspace_move_alloc(allocman_t *alloc, cspacepath_t src, cspacepath_t *result) {
    int error;
    error = allocman_cspace_alloc(alloc, result);
    if (error) {
        return error;
    }
    return vka_cnode_move(result, &src) != seL4_NoError;
}

static inline int cspace_move_alloc_cptr(allocman_t *alloc, cspace_interface_t source_cspace, seL4_CPtr slot, cspacepath_t *result) {
    return cspace_move_alloc(alloc, source_cspace.make_path(source_cspace.cspace, slot), result);
}


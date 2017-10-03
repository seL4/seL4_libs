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

static inline void allocman_sel4_arch_configure_reservations(allocman_t *alloc) {
    allocman_configure_utspace_reserve(alloc, (struct allocman_utspace_chunk) {vka_get_object_size(seL4_X86_PageDirectoryObject, 0), seL4_X86_PageDirectoryObject, 1});
    allocman_configure_utspace_reserve(alloc, (struct allocman_utspace_chunk) {vka_get_object_size(seL4_X86_PDPTObject, 0), seL4_X86_PDPTObject, 1});
}


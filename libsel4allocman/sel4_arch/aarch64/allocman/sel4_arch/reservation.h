/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <allocman/allocman.h>

static inline void allocman_sel4_arch_configure_reservations(allocman_t *alloc) {
    allocman_configure_utspace_reserve(alloc, (struct allocman_utspace_chunk) {vka_get_object_size(seL4_ARM_PageDirectoryObject, 0), seL4_ARM_PageDirectoryObject, 1});
    allocman_configure_utspace_reserve(alloc, (struct allocman_utspace_chunk) {vka_get_object_size(seL4_ARM_PageUpperDirectoryObject, 0), seL4_ARM_PageUpperDirectoryObject, 1});
}


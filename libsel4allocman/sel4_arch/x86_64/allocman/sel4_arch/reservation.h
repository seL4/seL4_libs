/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __ALLOCMAN_SEL4_ARCH_RESERVATION__
#define __ALLOCMAN_SEL4_ARCH_RESERVATION__

#include <allocman/allocman.h>

static inline void allocman_sel4_arch_configure_reservations(allocman_t *alloc) {
    allocman_configure_utspace_reserve(alloc, (struct allocman_utspace_chunk) {vka_get_object_size(seL4_X86_PageDirectoryObject, 0), seL4_X86_PageDirectoryObject, 1});
    allocman_configure_utspace_reserve(alloc, (struct allocman_utspace_chunk) {vka_get_object_size(seL4_X86_PDPTObject, 0), seL4_X86_PDPTObject, 1});
}

#endif

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef _SEL4UTILS_SEL4_ARCH_MAPPING_H
#define _SEL4UTILS_SEL4_ARCH_MAPPING_H

#include <vspace/arch/page.h>
#include <vka/vka.h>
#include <vka/object.h>

static inline int
sel4utils_create_object_at_level(vka_t *vka, seL4_Word failed_bits, vka_object_t *objects, int *num_objects, void *vaddr, seL4_CPtr vspace_root)
{
    ZF_LOGE("TODO: Not implemented yet");
}

#endif /* _SEL4UTILS_SEL4_ARCH_MAPPING_H */

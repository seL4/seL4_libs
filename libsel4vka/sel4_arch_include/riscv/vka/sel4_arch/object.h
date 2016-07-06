/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _VKA_SEL4_ARCH_OBJECT_H__
#define _VKA_SEL4_ARCH_OBJECT_H__

#include <vka/vka.h>
#include <vka/kobject_t.h>
#include <utils/util.h>

/*
static inline int vka_alloc_pd(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, kobject_get_type(KOBJECT_PAGE_DIRECTORY, 0), seL4_PageTableBits, result);
}
*/
static inline int vka_alloc_pd(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, kobject_get_type(KOBJECT_PAGE_TABLE, 0), seL4_PageTableBits, result);
}

static inline int vka_alloc_vspace_root(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_pd(vka, result);
}

static inline unsigned long
vka_riscv_mode_get_object_size(seL4_Word objectType)
{
    switch (objectType) {
    default:
        /* Unknown object type. */
        ZF_LOGF("Unknown object type %ld", (long)objectType);
        return -1;
    }
}

#endif /* _VKA_SEL4_ARCH_OBJECT_H__ */

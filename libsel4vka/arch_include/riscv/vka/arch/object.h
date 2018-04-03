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
#ifndef _VKA_ARCH_OBJECT_H__
#define _VKA_ARCH_OBJECT_H__

#include <vka/vka.h>
#include <vka/kobject_t.h>
#include <utils/util.h>
#include <vka/sel4_arch/object.h>

static inline int vka_alloc_io_page_table(UNUSED vka_t *vka, UNUSED vka_object_t *result)
{
    return 0; /* Not supported */
}

LEAKY(io_page_table)

static inline unsigned long
vka_arch_get_object_size(seL4_Word objectType)
{
    switch (objectType) {
    case seL4_RISCV_4K_Page:
        return seL4_PageBits;
    case seL4_RISCV_Mega_Page:
        return seL4_LargePageBits;
#if CONFIG_PT_LEVELS > 2
    case seL4_RISCV_Giga_Page:
        return seL4_HugePageBits;
#endif
#if CONFIG_PT_LEVELS > 3
    case seL4_RISCV_Tera_Page:
        return seL4_TeraPageBits;
#endif
    case seL4_RISCV_PageTableObject:
        return seL4_PageTableBits;

    default:
        return vka_riscv_mode_get_object_size(objectType);
    }
}

#endif /* _VKA_ARCH_OBJECT_H__ */

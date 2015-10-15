/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _VKA_OBJECT_H__
#define _VKA_OBJECT_H__

#include <vka/vka.h>
#include <vka/kobject_t.h>

#ifdef CONFIG_X86_64

static inline int vka_alloc_page_map_level4(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, kobject_get_type(KOBJECT_PAGE_MAP_LEVEL4, 0), seL4_PageMapLevel4Bits, result);
}

static inline int vka_alloc_page_directory_pointer_table(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, kobject_get_type(KOBJECT_PAGE_DIRECTORY_POINTER_TABLE, 0), seL4_PageDirPointerTableBits, result);
}

#endif

LEAKY(page_map_level4)
LEAKY(page_directory_pointer_table)

/*
 * Get the size (in bits) of the untyped memory required to create an object of
 * the given size.
 */
static inline unsigned long
vka_arch_get_object_size(seL4_Word objectType)
{
    switch (objectType) {
    case seL4_X64_4K:
        return seL4_PageBits;
    case seL4_X64_2M:
        return 21;
    case seL4_X64_PageTableObject:
        return seL4_PageTableBits;
    case seL4_X64_PageDirectoryObject:
        return seL4_PageDirBits;
    case seL4_X64_PageDirectoryPointerTableObject:
        return seL4_PageDirPointerTableBits;
    case seL4_X64_PageMapLevel4Object:
        return seL4_PageMapLevel4Bits;
    default:
        ZF_LOGE("Unknown object type");
        return -1;
    }
}

#endif /* _VKA_OBJECT_H__ */

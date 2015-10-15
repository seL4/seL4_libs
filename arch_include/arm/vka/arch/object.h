/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _VKA_ARCH_OBJECT_H__
#define _VKA_ARCH_OBJECT_H__

#include <vka/vka.h>
#include <utils/util.h>

/*resource allocation interfaces for virtual machine extensions on ARM */
#ifdef ARM_HYP
static inline int vka_alloc_vcpu(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, seL4_ARM_VCPUObject, seL4_ARM_VCPUBits, result);
}
#endif /* ARM_HYP */

#ifdef ARM_HYP
LEAKY(vcpu)
#endif

/*
 * Get the size (in bits) of the untyped memory required to create an object of
 * the given size.
 */
static inline unsigned long
vka_arch_get_object_size(seL4_Word objectType)
{
    switch (objectType) {
    case seL4_ARM_SmallPageObject:
        return seL4_PageBits;
    case seL4_ARM_LargePageObject:
        return 16;
#ifdef ARM_HYP
    case seL4_ARM_SectionObject:
        return 21;
    case seL4_ARM_SuperSectionObject:
        return 25;
    case seL4_ARM_VCPUObject:
        return seL4_ARM_VCPUBits;
#else /* ARM_HYP */
    case seL4_ARM_SectionObject:
        return 20;
    case seL4_ARM_SuperSectionObject:
        return 24;
#endif /* ARM_HYP */
    case seL4_ARM_PageTableObject:
        return seL4_PageTableBits;
    case seL4_ARM_PageDirectoryObject:
        return seL4_PageDirBits;
    default:
        /* Unknown object type. */
        ZF_LOGF("Unknown object type");
        return -1;
    }
}

#endif /* _VKA_ARCH_OBJECT_H__ */

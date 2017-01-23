/*
 * Copyright 2016, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(D61_BSD)
 */

#ifndef _VKA_SEL4_ARCH_OBJECT_H__
#define _VKA_SEL4_ARCH_OBJECT_H__

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

static inline int vka_alloc_vspace_root(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_page_directory(vka, result);
}

#ifdef CONFIG_ARM_SMMU
static inline int vka_alloc_io_page_table(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, seL4_ARM_IOPageTableObject, seL4_IOPageTableBits, result);
}

LEAKY(io_page_table)
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
        return seL4_LargePageBits;
    case seL4_ARM_SectionObject:
        return seL4_SectionBits;
    case seL4_ARM_SuperSectionObject:
        return seL4_SuperSectionBits;
    case seL4_ARM_VCPUObject:
        return seL4_ARM_VCPUBits;
    case seL4_ARM_PageTableObject:
        return seL4_PageTableBits;
    case seL4_ARM_PageDirectoryObject:
        return seL4_PageDirBits;
#ifdef CONFIG_ARM_SMMU
    case seL4_ARM_IOPageTableObject:
        return seL4_IOPageTableBits;
#endif
    default:
        /* Unknown object type. */
        ZF_LOGF("Unknown object type");
        return -1;
    }
}

#endif /* _VKA_SEL4_ARCH_OBJECT_H__ */

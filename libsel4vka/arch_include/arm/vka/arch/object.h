/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <vka/vka.h>
#include <vka/kobject_t.h>
#include <utils/util.h>
#include <vka/sel4_arch/object.h>

/*resource allocation interfaces for virtual machine extensions on ARM */
#ifdef CONFIG_ARM_HYPERVISOR_SUPPORT
static inline int vka_alloc_vcpu(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, seL4_ARM_VCPUObject, seL4_ARM_VCPUBits, result);
}

LEAKY(vcpu)
#endif

#ifdef CONFIG_TK1_SMMU
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
    case seL4_ARM_PageTableObject:
        return seL4_PageTableBits;
    case seL4_ARM_PageDirectoryObject:
        return seL4_PageDirBits;
    case seL4_ARM_VCPUObject:
        return seL4_ARM_VCPUBits;
#ifdef CONFIG_TK1_SMMU
    case seL4_ARM_IOPageTableObject:
        return seL4_IOPageTableBits;
#endif
    default:
        return vka_arm_mode_get_object_size(objectType);;
    }
}


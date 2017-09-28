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

#include <vka/vka.h>
#include <vka/kobject_t.h>
#include <utils/util.h>
#include <vka/sel4_arch/object.h>

/*resource allocation interfaces for virtual machine extensions on x86*/
#ifdef CONFIG_VTX
static inline int vka_alloc_vcpu (vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, seL4_X86_VCPUObject, seL4_X86_VCPUBits, result);
}

static inline int vka_alloc_ept_page_directory_pointer_table (vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, seL4_X86_EPTPDPTObject, seL4_X86_EPTPDPTBits, result);
}

static inline int vka_alloc_ept_page_directory (vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, seL4_X86_EPTPDObject, seL4_X86_EPTPDBits, result);
}

static inline int vka_alloc_ept_page_table (vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, seL4_X86_EPTPTObject, seL4_X86_EPTPTBits, result);
}
static inline int vka_alloc_ept_pdpt(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, seL4_X86_EPTPDPTObject, seL4_X86_EPTPDPTBits, result);
}
static inline int vka_alloc_ept_pml4(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, seL4_X86_EPTPML4Object, seL4_X86_EPTPML4Bits, result);
}
#endif /* CONFIG_VTX */

static inline int vka_alloc_io_page_table(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, seL4_X86_IOPageTableObject, seL4_IOPageTableBits, result);
}

LEAKY(io_page_table)

#ifdef CONFIG_VTX
LEAKY(vcpu)
LEAKY(ept_page_directory_pointer_table)
LEAKY(ept_page_directory)
LEAKY(ept_page_table)
LEAKY(ept_pdpt)
LEAKY(ept_pml4)
#endif /* CONFIG_VTX */

static inline unsigned long
vka_arch_get_object_size(seL4_Word objectType)
{
    switch (objectType) {
    case seL4_X86_4K:
        return seL4_PageBits;
    case seL4_X86_LargePageObject:
        return seL4_LargePageBits;
    case seL4_X86_PageTableObject:
        return seL4_PageTableBits;
    case seL4_X86_PageDirectoryObject:
        return seL4_PageDirBits;

#ifdef CONFIG_VTX
    case seL4_X86_VCPUObject:
        return seL4_X86_VCPUBits;
    case seL4_X86_EPTPDPTObject:
        return seL4_X86_EPTPDPTBits;
    case seL4_X86_EPTPDObject:
        return seL4_X86_EPTPDBits;
    case seL4_X86_EPTPTObject:
        return seL4_X86_EPTPTBits;
    case seL4_X86_EPTPML4Object:
        return seL4_X86_EPTPML4Bits;
#endif

    case seL4_X86_IOPageTableObject:
        return seL4_IOPageTableBits;

    default:
        return vka_x86_mode_get_object_size(objectType);
    }
}


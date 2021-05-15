/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <autoconf.h>
#include <stddef.h>
#include <utils/attribute.h>
#include <sel4/sel4.h>

/* ordered list of page sizes for this architecture */
static const UNUSED size_t sel4_page_sizes[] = {
    seL4_PageBits,
    seL4_LargePageBits,
#if defined(CONFIG_ARCH_AARCH32)
    seL4_SectionBits,
    seL4_SuperSectionBits
#elif defined(CONFIG_ARCH_AARCH64)
    seL4_HugePageBits
#endif
};

#define seL4_ARCH_Uncached_VMAttributes 0

#define seL4_ARCH_Page_Map             seL4_ARM_Page_Map
#define seL4_ARCH_Page_MapIO           seL4_ARM_Page_MapIO
#define seL4_ARCH_Page_Unmap           seL4_ARM_Page_Unmap
#define seL4_ARCH_Page_GetAddress      seL4_ARM_Page_GetAddress
#define seL4_ARCH_Page_GetAddress_t    seL4_ARM_Page_GetAddress_t
#define seL4_ARCH_PageTable_Map        seL4_ARM_PageTable_Map
#define seL4_ARCH_IOPageTable_Map      seL4_ARM_IOPageTable_Map
#define seL4_ARCH_PageTable_Unmap      seL4_ARM_PageTable_Unmap
#define seL4_ARCH_ASIDPool_Assign      seL4_ARM_ASIDPool_Assign
#define seL4_ARCH_ASIDControl_MakePool seL4_ARM_ASIDControl_MakePool
#define seL4_ARCH_PageTableObject      seL4_ARM_PageTableObject
#define seL4_ARCH_PageDirectoryObject  seL4_ARM_PageDirectoryObject
#define seL4_ARCH_Default_VMAttributes seL4_ARM_Default_VMAttributes
#define seL4_ARCH_VMAttributes         seL4_ARM_VMAttributes
#define seL4_ARCH_4KPage               seL4_ARM_SmallPageObject
#define seL4_ARCH_LargePageObject      seL4_ARM_LargePageObject
/* for the size of a large page use seL4_LargePageBits */
#define ARCHPageGetAddress             ARMPageGetAddress


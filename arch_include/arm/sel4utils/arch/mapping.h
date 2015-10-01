/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef _SEL4UTILS_ARCH_MAPPING_H
#define _SEL4UTILS_ARCH_MAPPING_H

#define seL4_ARCH_Uncached_VMAttributes 0

#define seL4_ARCH_Page_Map             seL4_ARM_Page_Map
#define seL4_ARCH_Page_Unmap           seL4_ARM_Page_Unmap
#define seL4_ARCH_Page_GetAddress      seL4_ARM_Page_GetAddress
#define seL4_ARCH_Page_GetAddress_t    seL4_ARM_Page_GetAddress_t
#define seL4_ARCH_PageTable_Map        seL4_ARM_PageTable_Map
#define seL4_ARCH_PageTable_Unmap      seL4_ARM_PageTable_Unmap
#define seL4_ARCH_ASIDPool_Assign      seL4_ARM_ASIDPool_Assign
#define seL4_ARCH_ASIDControl_MakePool seL4_ARM_ASIDControl_MakePool
#define seL4_ARCH_PageTableObject      seL4_ARM_PageTableObject
#define seL4_ARCH_PageDirectoryObject  seL4_ARM_PageDirectoryObject
#define seL4_ARCH_Default_VMAttributes seL4_ARM_Default_VMAttributes
#define seL4_ARCH_VMAttributes         seL4_ARM_VMAttributes
#define seL4_ARCH_4KPage               seL4_ARM_SmallPageObject
/* Remap does not exist on all kernels */
#define seL4_ARCH_Page_Remap           seL4_ARM_Page_Remap
#define ARCHPageGetAddress             ARMPageGetAddress

#endif /* _SEL4UTILS_ARCH_MAPPING_H */

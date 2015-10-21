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


#define seL4_ARCH_Page_Map             seL4_IA32_Page_Map
#define seL4_ARCH_Page_Unmap           seL4_IA32_Page_Unmap
#define seL4_ARCH_Page_GetAddress      seL4_IA32_Page_GetAddress
#define seL4_ARCH_Page_GetAddress_t    seL4_IA32_Page_GetAddress_t
#define seL4_ARCH_PageTable_Map        seL4_IA32_PageTable_Map
#define seL4_ARCH_PageTable_Unmap      seL4_IA32_PageTable_Unmap
#define seL4_ARCH_ASIDPool_Assign      seL4_IA32_ASIDPool_Assign
#define seL4_ARCH_ASIDControl_MakePool seL4_IA32_ASIDControl_MakePool
#define seL4_ARCH_PageTableObject      seL4_IA32_PageTableObject
#define seL4_ARCH_PageDirectoryObject  seL4_IA32_PageDirectoryObject
#define seL4_ARCH_Default_VMAttributes seL4_IA32_Default_VMAttributes
#define seL4_ARCH_VMAttributes         seL4_IA32_VMAttributes
#define seL4_ARCH_4KPage               seL4_IA32_4K
#define seL4_ARCH_Uncached_VMAttributes seL4_IA32_CacheDisabled
/* Remap does not exist on all kernels */
#define seL4_ARCH_Page_Remap           seL4_IA32_Page_Remap
#define ARCHPageGetAddress             IA32PageGetAddress

#endif /* _SEL4UTILS_ARCH_MAPPING_H */

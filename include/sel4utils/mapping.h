/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _UTILS_MAPPING_H_
#define _UTILS_MAPPING_H_

#include <utils/util.h>
#include <sel4/sel4.h>
#include <autoconf.h>


#if defined(CONFIG_ARCH_ARM)

/* sizes (in bits) of pages supported by sel4 in ascending order */
#if defined(ARM_HYP)
static int UNUSED sel4_supported_page_sizes[] = {12, 16, 21, 25};
#else
static int UNUSED sel4_supported_page_sizes[] = {12, 16, 20, 24};
#endif

#define seL4_ARCH_Uncached_VMAttributes 0

#define seL4_ARCH_Page_Map             seL4_ARM_Page_Map
#define seL4_ARCH_Page_Unmap           seL4_ARM_Page_Unmap
#define seL4_ARCH_PageTable_Map        seL4_ARM_PageTable_Map
#define seL4_ARCH_ASIDPool_Assign      seL4_ARM_ASIDPool_Assign
#define seL4_ARCH_PageTableObject      seL4_ARM_PageTableObject
#define seL4_ARCH_PageDirectoryObject  seL4_ARM_PageDirectoryObject
#define seL4_ARCH_Default_VMAttributes seL4_ARM_Default_VMAttributes
#define seL4_ARCH_VMAttributes         seL4_ARM_VMAttributes
#define seL4_ARCH_4KPage               seL4_ARM_SmallPageObject

#elif defined(CONFIG_X86_64)

/* sizes (in bits) of pages supported by sel4 in ascending order */
static int UNUSED sel4_supported_page_sizes[] = {12, 21};

#define seL4_ARCH_Page_Unmap                    seL4_X64_Page_Unmap
#define seL4_ARCH_Page_Map                      seL4_X64_Page_Map
#define seL4_ARCH_PageTable_Map                 seL4_X64_PageTable_Map
#define seL4_ARCH_PageDirectory_Map             seL4_X64_PageDirectory_Map
#define seL4_ARCH_PageDirectoryPointerTable_Map seL4_X64_PageDirectoryPointerTable_Map
#define seL4_CapInitThreadPD                    seL4_CapInitThreadPML4
#define seL4_ARCH_VMAttributes                  seL4_IA32_VMAttributes
#define seL4_ARCH_4KPage                        seL4_X64_4K
#define seL4_ARCH_Default_VMAttributes          seL4_IA32_Default_VMAttributes

#elif defined(CONFIG_ARCH_IA32) /* CONFIG_ARCH_ARM */

/* sizes (in bits) of pages supported by sel4 in ascending order */
static int UNUSED sel4_supported_page_sizes[] = {12, 22};

#define seL4_ARCH_Page_Map             seL4_IA32_Page_Map
#define seL4_ARCH_Page_Unmap           seL4_IA32_Page_Unmap
#define seL4_ARCH_PageTable_Map        seL4_IA32_PageTable_Map
#define seL4_ARCH_ASIDPool_Assign      seL4_IA32_ASIDPool_Assign
#define seL4_ARCH_PageTableObject      seL4_IA32_PageTableObject
#define seL4_ARCH_PageDirectoryObject  seL4_IA32_PageDirectoryObject
#define seL4_ARCH_Default_VMAttributes seL4_IA32_Default_VMAttributes
#define seL4_ARCH_VMAttributes         seL4_IA32_VMAttributes
#define seL4_ARCH_4KPage               seL4_IA32_4K
#define seL4_ARCH_Uncached_VMAttributes 0

#endif /* CONFIG_ARCH_IA32 */

#define NUM_SEL4_PAGE_SIZES ((int) ARRAY_SIZE(sel4_supported_page_sizes))

#ifdef CONFIG_LIB_SEL4_VKA

#include <vka/vka.h>
#include <vka/object.h>

/* Map a page to a virtual address, allocating a page table if neccessary.
*
*
* @param vka a vka compliant allocator
* @param pd page directory to map the page into
* @param page capability to the page to map in
* @param vaddr unmapped virutal address to map the page into
* @param rights permissions to map the page with
* @param cacheable 1 if the page should be cached (0 if it is for DMA)
* @param pagetable empty vka_object_t structure to be populated with page table
*                  info if one is allocated
*
* @return error sel4 error code or -1 if allocation failed.
*/
int sel4utils_map_page(vka_t *vka, seL4_CPtr pd, seL4_CPtr frame, void *vaddr,
        seL4_CapRights rights, int cacheable, vka_object_t *pagetable);

/** convenient wrapper this if you don't want to track allocated page tables */
static inline int sel4utils_map_page_leaky(vka_t *vka, seL4_CPtr pd, seL4_CPtr frame, void *vaddr,
        seL4_CapRights rights, int cacheable)
{
    vka_object_t pagetable;
    return sel4utils_map_page(vka, pd, frame, vaddr, rights, cacheable, &pagetable);
}

#endif /* CONFIG_LIB_SEL4_VKA */

#ifdef CONFIG_ARCH_IA32

#ifdef CONFIG_IOMMU
int sel4utils_map_iospace_page(vka_t *vka, seL4_CPtr iospace, seL4_CPtr frame, seL4_Word vaddr,
                         seL4_CapRights rights, int cacheable, seL4_Word size_bits,
                         vka_object_t *pts, int *num_pts);
#endif

#ifdef CONFIG_VTX
int sel4utils_map_ept_page(vka_t *vka, seL4_CPtr pd, seL4_CPtr frame, seL4_Word vaddr,
                   seL4_CapRights rights, int cacheable, seL4_Word size_bits, vka_object_t *pagetable, vka_object_t *pagedir);

#endif 
#endif 



#endif /* UTILS_MAPPING_H_ */

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef SEL4UTILS_IOMMU_DMA_H
#define SEL4UTILS_IOMMU_DMA_H

#include <autoconf.h>

#if defined(CONFIG_LIB_SEL4_VSPACE) && defined(CONFIG_LIB_SEL4_VKA) && defined(CONFIG_LIB_PLATSUPPORT) && defined(CONFIG_IOMMU)

#include <sel4/sel4.h>
#include <vka/vka.h>
#include <vspace/vspace.h>
#include <platsupport/io.h>

/**
 * Creates an implementation of a dma manager that is designed to work only in the presence of an IOMMU
 * Due to its reliance on malloc for actually allocating memory blocks, and because malloc'ed memory is
 * assumed to be cached, it only supports cached allocations. The basic operation of this allocator is to
 * malloc block of memory, find the frame capabilities, duplicate and map these into the appropriate
 * iospaces. It is the responsibility of the user to ensure that whatever frames are used in mapping
 * the malloc region are of a size that the hardware will accept being mapped into the IOMMU
 * @param vka Allocation interface used for allocated cslots and any required paging structures for the iospace.
 *            This interface will be copied
 * @param vspace Virtual memory manager that needs to contain any mappings used to back malloc.
 *            This interface will be copied
 * @param dma_man Pointer to dma manager struct that will be filled out
 * @param num_iospaces Number of iospaces we wish allocations from this dma manager to be valid in
 * @param iospaces Pointer to list of cptrs representing the iospaces to map into
 * @return 0 on success
 */
int sel4utils_make_iommu_dma_alloc(vka_t *vka, vspace_t *vspace, ps_dma_man_t *dma_man, unsigned int num_iospaces, seL4_CPtr *iospaces);

#endif /* CONFIG_LIB_SEL4_VSPACE && CONFIG_LIB_SEL4_VKA && CONFIG_LIB_PLATSUPPORT && CONFIG_IOMMU */
#endif /* SEL4_UTILS_IOMMU_DMA_H */

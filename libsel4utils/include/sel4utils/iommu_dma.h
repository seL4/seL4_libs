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

#include <autoconf.h>

#if defined(CONFIG_IOMMU)

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

/**
 * Variant of ps_dma_alloc that allows the caller to allocate memory in their address space for use
 * as the dma buffer. This function takes a description of a region of (virtual) memory, and maps
 * the frames that back the region into the dma manager's iospace such that the iovaddr of each frame
 * corresponds to its vaddr.
 *
 * The intended use case for this function is in environments without a dynamic heap (that is, where
 * malloc is not backed by a vspace). The dma_man argument must be a pointer to a dma manager that
 * was created using sel4utils_make_iommu_dma_alloc. The vspace argument to sel4utils_make_iommu_dma_alloc
 * must be able to resolve vaddrs within the specified buffer to frame caps with its get_cap function.
 *
 * This function takes a cookie rather than a dma manager to allow it to be used in the implementation
 * of a ps_dma_alloc_fn_t.
 *
 * @param dma_cookie The cookie of a dma manager initialised with sel4utils_make_iommu_dma_alloc.
 * @param vaddr A pointer to a region of memory to use as the dma buffer. The vspace given to
 *              sel4utils_make_iommu_dma_alloc to initialise dma_man must be able to resolve
 *              addresses in this buffer to frame caps with its get_cap function.
 * @param size The size of the region of memory pointed to by vaddr.
 * @return 0 on success
 */
int sel4utils_iommu_dma_alloc_iospace(void *dma_cookie, void *vaddr, size_t size);
#endif /* CONFIG_IOMMU */

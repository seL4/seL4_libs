/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */
/*
 *  Simple allocator which "steals" some contiguous host memory and allocates physically contiguous
 *  pages incrementally with no free support.
 *
 *  Used by the VMM to map pages into guest in a 1:1 mapping to support DMA, as a fallback when
 *  the IOMMU is not available. Used to map the kernel ELF binary + initrd binary regions, as these
 *  must be a big continuous physical block of memory.
 *
 *  Authors:
 *      Xi (Ma) Chen
 */

#ifndef _LIB_VMM_PLAT_PHYS_ALLOC_H_
#define _LIB_VMM_PLAT_PHYS_ALLOC_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sel4/sel4.h>
#include "vmm/config.h"
#include <simple/simple.h>

#if defined(LIB_VMM_GUEST_DMA_ONE_TO_ONE) || defined(LIB_VMM_GUEST_DMA_IOMMU)

/* This bootstrapping is quite hacky and it makes a lot of assumptions on cspace layout.
 * In particular it assumes a single level cspace of the form a rootserver would be given */
uint32_t pframe_alloc_init(simple_t *simple, uint32_t paddr_min,
                           uint32_t size, uint32_t page_size_bits, bool *stole_untyped_list, seL4_CPtr *free_cptr);

int pframe_alloc(vka_t* vka, seL4_CPtr* out_cap, uint32_t* out_paddr);

uint32_t pframe_skip_to_aligned(uint32_t align, uint32_t size_left_min_check);

#endif /* LIB_VMM_GUEST_DMA_ONE_TO_ONE */
#endif /* _LIB_VMM_PLAT_PHYS_ALLOC_H_ */

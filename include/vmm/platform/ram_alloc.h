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
 * Simple allocator which contiguously allocates untypeds using another allocator, and then
 * allocates frames out of that. Requires that the underlying allocator supports reporting physical
 * addresses of allocated untypes.
 *
 * The major different between ram_alloc and phys_allooc is that phys_alloc reserves untypeds at
 * boot in order to guarantee continguous memory, while ram_alloc allocates untypeds dynamically.
 *
 * Authors:
 *      Xi (Ma) Chen
 */

#ifndef _LIB_VMM_PLAT_GUEST_RAM_ALLOC_H_
#define _LIB_VMM_PLAT_GUEST_RAM_ALLOC_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sel4/sel4.h>
#include <allocman/allocman.h>
#include <vka/vka.h>
#include "vmm/config.h"

#if defined(LIB_VMM_GUEST_DMA_ONE_TO_ONE) || defined(LIB_VMM_GUEST_DMA_IOMMU)

void guest_ram_alloc_init(allocman_t* allocman);

int guest_ram_frame_alloc(vka_t* vka, seL4_CPtr* out_cap, uint32_t* out_paddr);

#endif /* LIB_VMM_GUEST_DMA_ONE_TO_ONE */
#endif /* _LIB_VMM_PLAT_GUEST_RAM_ALLOC_H_ */

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef SEL4UTILS_PAGE_DMA_H
#define SEL4UTILS_PAGE_DMA_H

#include <autoconf.h>

#if defined(CONFIG_LIB_SEL4_VSPACE) && defined(CONFIG_LIB_SEL4_VKA) && defined(CONFIG_LIB_PLATSUPPORT)

#include <sel4/sel4.h>
#include <vka/vka.h>
#include <vspace/vspace.h>
#include <platsupport/io.h>

/**
 * Creates an implementation of a dma manager that is designed to allocate at page granularity. Due
 * to implementation details it will round up all allocations to the next power of 2, or 4k (whichever
 * is larger). This allocator will put mappings into the vspace with custom cookie values and you
 * must free all dma allocations before tearing down the vspace
 * @param vka Allocation interface for allocating untypeds (for frames) and slots
 * @param vspace Virtual memory manager used for mapping frames
 * @param dma_man Pointer to dma manager struct that will be filled out
 * @return 0 on success
 */
int sel4utils_new_page_dma_alloc(vka_t *vka, vspace_t *vspace, ps_dma_man_t *dma_man);

#endif /* CONFIG_LIB_SEL4_VSPACE && CONFIG_LIB_SEL4_VKA && CONFIG_LIB_PLATSUPPORT */
#endif /* SEL4_UTILS_PAGE_DMA_H */

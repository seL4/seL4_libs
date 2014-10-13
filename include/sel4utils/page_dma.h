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
 * Creates an implementation of a dma manager that is designed to allocate in a single page granularity.
 * This means that allocations < 1 page will be rounded up, and allocations > 1 page will always fail.
 * @param vka Allocation interface that needs to be the same one as used by the vspace for the
              unmapping of pages
 * @param vspace Virtual memory manager used for creating frames
 * @param dma_man Pointer to dma manager struct that will be filled out
 * @return 0 on success
 */
int sel4utils_new_page_dma_alloc(vka_t *vka, vspace_t *vspace, ps_dma_man_t *dma_man);

#endif /* CONFIG_LIB_SEL4_VSPACE && CONFIG_LIB_SEL4_VKA && CONFIG_LIB_PLATSUPPORT */
#endif /* SEL4_UTILS_PAGE_DMA_H */

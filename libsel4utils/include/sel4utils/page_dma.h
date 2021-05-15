/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <autoconf.h>
#include <sel4utils/gen_config.h>

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


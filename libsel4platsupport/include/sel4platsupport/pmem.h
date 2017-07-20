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

#include <stdint.h>
#include <simple/simple.h>
#include <platsupport/pmem.h>
#include <vka/object.h>

typedef struct {
    pmem_region_t region;
    vka_object_t obj;
} sel4ps_pmem_t;

/**
 * Returns number of physical memory regions.  Each platform has a specific implementation
 * @param  simple      libsel4simple implementation
 * @return             Number of regions or -1 on error.
 */
int sel4platsupport_get_num_pmem_regions(simple_t *simple);

/**
 * Returns an array of physical memory regions.  Each platform has a specific implementation
 * @param  simple      libsel4simple implementation
 * @param  max_length  max length of region array
 * @param  region_list pointer to region array
 * @return             Number of regions or -1 on error.
 */
int sel4platsupport_get_pmem_region_list(simple_t *simple, size_t max_length, pmem_region_t *region_list);

/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <sel4platsupport/pmem.h>
#include <utils/util.h>

int sel4platsupport_get_num_pmem_regions(UNUSED simple_t *simple) {
    return 0;
}

int sel4platsupport_get_pmem_region_list(UNUSED simple_t *simple, UNUSED size_t max_length, UNUSED pmem_region_t *region_list) {
    ZF_LOGW("Not implemented");
    return -1;
}

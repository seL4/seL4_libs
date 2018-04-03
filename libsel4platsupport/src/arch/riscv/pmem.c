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

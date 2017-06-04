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
#include <sel4/sel4.h>
#include <sel4/arch/bootinfo_types.h>
#include <sel4platsupport/pmem.h>
#include <utils/util.h>

int sel4platsupport_get_num_pmem_regions(simple_t *simple) {
    seL4_X86_BootInfo_mmap_t data;
    int error = simple_get_extended_bootinfo(simple, SEL4_BOOTINFO_HEADER_X86_MBMMAP, &data, sizeof(seL4_X86_BootInfo_mmap_t));
    if (error == -1) {
        ZF_LOGE("Could not find info");
        return 0;
    }

    return data.mmap_length/sizeof(data.mmap[0]);
}

int sel4platsupport_get_pmem_region_list(simple_t *simple, size_t max_length, pmem_region_t *region_list) {
    seL4_X86_BootInfo_mmap_t data;
    int error = simple_get_extended_bootinfo(simple, SEL4_BOOTINFO_HEADER_X86_MBMMAP, &data, sizeof(seL4_X86_BootInfo_mmap_t));
    if (error == -1) {
        ZF_LOGE("Could not find info");
        return -1;
    }
    seL4_X86_mb_mmap_t *mmap = (seL4_X86_mb_mmap_t *)((unsigned long)data.mmap);
    size_t i = 0;
    for (i = 0; i < max_length && i < (data.mmap_length/sizeof(mmap[0])); i++) {
        region_list[i].type = mmap[i].type == SEL4_MULTIBOOT_RAM_REGION_TYPE ? PMEM_TYPE_RAM : PMEM_TYPE_UNKNOWN;
        region_list[i].base_addr = mmap[i].base_addr;
        region_list[i].length  = mmap[i].length;
    }
    return i;
}

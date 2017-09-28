/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#pragma once

#include <stdint.h>
#include <vspace/vspace.h>

typedef struct guest_ram_region {
    /* Guest physical start address */
    uintptr_t start;
    /* size in bytes */
    size_t size;
    /* whether or not this region has been 'allocated'
     * an allocated region has initial guest information loaded into it
     * this could be elf, initrd modules, etc */
    int allocated;
} guest_ram_region_t;

typedef struct guest_memory {
    /* Guest vspace management. This manages ALL mappings in the guest
     * address space. This may include memory that we may tell the guest
     * is not present/reserved */
    vspace_t vspace;
    /* We maintain all pieces of ram as a sorted list of regions.
     * This is memory that we will specifically give the guest as actual RAM */
    int num_ram_regions;
    guest_ram_region_t *ram_regions;
} guest_memory_t;

struct vmm;

uintptr_t guest_ram_largest_free_region_start(guest_memory_t *guest_memory);
void print_guest_ram_regions(guest_memory_t *guest_memory);
void guest_ram_mark_allocated(guest_memory_t *guest_memory, uintptr_t start, size_t bytes);
uintptr_t guest_ram_allocate(guest_memory_t *guest_memory, size_t bytes);

int vmm_alloc_guest_device_at(struct vmm *vmm, uintptr_t start, size_t bytes);
uintptr_t vmm_map_guest_device(struct vmm *vmm, uintptr_t paddr, size_t bytes, size_t align);
int vmm_map_guest_device_at(struct vmm *vmm, uintptr_t vaddr, uintptr_t paddr, size_t bytes);
int vmm_alloc_guest_ram_at(struct vmm *vmm, uintptr_t start, size_t bytes);
int vmm_alloc_guest_ram(struct vmm *vmm, size_t bytes, int onetoone);


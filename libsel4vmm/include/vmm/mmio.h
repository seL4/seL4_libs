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

typedef struct vmm_vcpu vmm_vcpu_t;

// Deals with ranges of memory mapped io for emulated devices

typedef void (*vmm_mmio_read_fn)(vmm_vcpu_t * vcpu, void *cookie,
        uint32_t offset, int size, uint32_t *result);
typedef void (*vmm_mmio_write_fn)(vmm_vcpu_t *vcpu, void *cookie,
        uint32_t offset, int size, uint32_t value);

typedef struct vmm_mmio_range {
    uintptr_t start;
    uintptr_t end;

    vmm_mmio_read_fn read_handler;
    vmm_mmio_write_fn write_handler;

    void *cookie;

    const char *name;
} vmm_mmio_range_t;

typedef struct vmm_mmio_list {
    vmm_mmio_range_t *ranges; // Sorted array
    int num_ranges;
} vmm_mmio_list_t;

// Initialise
int vmm_mmio_init(vmm_mmio_list_t *list);

// Try to handle an exit with mmio
// Returns 0 if handled, or -1 otherwise
int vmm_mmio_exit_handler(vmm_vcpu_t *vcpu, uintptr_t addr, unsigned int qualification);

int vmm_mmio_add_handler(vmm_mmio_list_t *list, uintptr_t start, uintptr_t end,
        void *cookie, const char *name,
        vmm_mmio_read_fn read_handler, vmm_mmio_write_fn write_handler);


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
#include <pci/pci.h>

#include "vmm/driver/pci.h"
#include "vmm/vmm.h"

/* Struct definition of a PCI device. This is used for emulating a device from
 * purely memory reads. This is not generally useful on its own, but provides
 * a nice skeleton */
typedef struct vmm_pci_device_def {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t revision_id;
    uint8_t prog_if;
    uint8_t subclass;
    uint8_t class_code;
    uint8_t cache_line_size;
    uint8_t latency_timer;
    uint8_t header_type;
    uint8_t bist;
    uint32_t bar0;
    uint32_t bar1;
    uint32_t bar2;
    uint32_t bar3;
    uint32_t bar4;
    uint32_t bar5;
    uint32_t cardbus;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;
    uint32_t expansion_rom;
    uint8_t caps_pointer;
    uint8_t reserved1;
    uint16_t reserved2;
    uint32_t reserved3;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint8_t min_grant;
    uint8_t max_latency;
    /* Now additional pointer to arbitrary capabilities */
    int caps_len;
    void *caps;
} __attribute__((packed)) vmm_pci_device_def_t;

typedef struct vmm_pci_bar {
    int ismem;
    /* Address must be size aligned */
    uintptr_t address;
    int size_bits;
    /* only if memory */
    int prefetchable;
} vmm_pci_bar_t;

/* Helper write function that just ignores any writes */
int vmm_pci_entry_ignore_write(void *cookie, int offset, int size, uint32_t value);

/* Read and write methods for a memory device */
int vmm_pci_mem_device_read(void *cookie, int offset, int size, uint32_t *result);
int vmm_pci_mem_device_write(void *cookie, int offset, int size, uint32_t value);

void define_pci_host_bridge(vmm_pci_device_def_t *bridge);

/* Construct a pure passthrough device based on the real PCI. This is almost always useless as
 * you will almost certainly want to rebase io memory */
vmm_pci_entry_t vmm_pci_create_passthrough(vmm_pci_address_t addr, vmm_pci_config_t config);

/* Bar read/write emulation, rest passed on */
vmm_pci_entry_t vmm_pci_create_bar_emulation(vmm_pci_entry_t existing, int num_bars, vmm_pci_bar_t *bars);

/* Interrupt read/write emulation, rest passed on */
vmm_pci_entry_t vmm_pci_create_irq_emulation(vmm_pci_entry_t existing, int irq);

/* Capability space emulation. Takes list of addresses to use to form a capability
 * linked list, as well as a ranges of the capability space that should be
 * directly disallowed. Assumes a type 0 device.
 */
vmm_pci_entry_t vmm_pci_create_cap_emulation(vmm_pci_entry_t existing, int num_caps, uint8_t *caps, int num_ranges, uint8_t *range_starts, uint8_t *range_ends);

/* Finds the MSI capabilities and uses vmm_pci_create_cap_emulation to remove them */
vmm_pci_entry_t vmm_pci_no_msi_cap_emulation(vmm_pci_entry_t existing);

/* Takes a libpci device scan and adds the bar resources to the guest, creating
 * new pci bar information that can be passed to a virtual config space creator.
 * Creates at most 6 bars, returns how many it created, negative on error */
int vmm_pci_helper_map_bars(vmm_t *vmm, libpci_device_iocfg_t *cfg, vmm_pci_bar_t *bars);


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
/* x86 VMM PCI Driver, which manages the host's PCI devices, and handles guest OS PCI config space
 * read & writes.
*/

#pragma once

#include <stdint.h>

typedef struct vmm_pci_address {
    uint8_t bus;
    uint8_t dev;
    uint8_t fun;
} vmm_pci_address_t;

/* Functions for accessing the pci config space */
typedef struct vmm_pci_config {
    void *cookie;
    uint8_t (*ioread8)(void *cookie, vmm_pci_address_t addr, unsigned int offset);
    uint16_t (*ioread16)(void *cookie, vmm_pci_address_t addr, unsigned int offset);
    uint32_t (*ioread32)(void *cookie, vmm_pci_address_t addr, unsigned int offset);
    void (*iowrite8)(void *cookie, vmm_pci_address_t addr, unsigned int offset, uint8_t val);
    void (*iowrite16)(void *cookie, vmm_pci_address_t addr, unsigned int offset, uint16_t val);
    void (*iowrite32)(void *cookie, vmm_pci_address_t addr, unsigned int offset, uint32_t val);
} vmm_pci_config_t;

/* Abstract virtual PCI thing. Could be a device or anything. This
 * can be inserted into the virtual PCI configuration space */
typedef struct vmm_pci_entry {
    /* Callback functions for reading or writing its configuration space.
     * Returns 0 on success, nonzero if an error occured */
    void *cookie;
    int (*ioread)(void *cookie, int offset, int size, uint32_t *result);
    int (*iowrite)(void *cookie, int offset, int size, uint32_t value);
} vmm_pci_entry_t;

typedef struct vmm_pci_space {
    /* Only support one bus at the moment. */
    vmm_pci_entry_t *bus0[32][8];
    /* For IO port emulation this is the current config address */
    uint32_t conf_port_addr;
} vmm_pci_space_t;

/* Initialize PCI space */
int vmm_pci_init(vmm_pci_space_t *space);

/* Add a PCI entry. Optionally reports where it is located */
int vmm_pci_add_entry(vmm_pci_space_t *space, vmm_pci_entry_t entry, vmm_pci_address_t *addr);

/* Functions for emulating PCI config spaces over IO ports */
int vmm_pci_io_port_in(void *cookie, unsigned int port_no, unsigned int size, unsigned int *result);
int vmm_pci_io_port_out(void *cookie, unsigned int port_no, unsigned int size, unsigned int value);


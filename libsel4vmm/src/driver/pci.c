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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sel4/sel4.h>
#include <pci/pci.h>
#include <pci/helper.h>

#include "vmm/debug.h"
#include "vmm/vmm.h"
#include "vmm/driver/pci.h"
#include "vmm/driver/pci_helper.h"

int vmm_pci_init(vmm_pci_space_t *space) {
    for (int i = 0; i < 32; i++) {
        for(int j = 0; j < 8; j++) {
            space->bus0[i][j] = NULL;
        }
    }
    space->conf_port_addr = 0;
    /* Define the initial PCI bridge */
    vmm_pci_device_def_t *bridge = malloc(sizeof(*bridge));
    if (!bridge) {
        ZF_LOGE("Failed to malloc memory for pci bridge");
        return -1;
    }
    define_pci_host_bridge(bridge);
    return vmm_pci_add_entry(space, (vmm_pci_entry_t){.cookie = bridge, .ioread = vmm_pci_mem_device_read, .iowrite = vmm_pci_entry_ignore_write}, NULL);
}

int vmm_pci_add_entry(vmm_pci_space_t *space, vmm_pci_entry_t entry, vmm_pci_address_t *addr) {
    /* Find empty dev */
    for (int i = 0; i < 32; i++) {
        if (!space->bus0[i][0]) {
            /* Allocate an entry */
            space->bus0[i][0] = malloc(sizeof(entry));
            *space->bus0[i][0] = entry;
            /* Report addr if reqeusted */
            if (addr) {
                *addr = (vmm_pci_address_t){.bus = 0, .dev = i, .fun = 0};
            }
            ZF_LOGI("Adding virtual PCI device at %02x:%02x.%d", 0, i, 0);
            return 0;
        }
    }
    ZF_LOGE("No free device slot on bus 0 to add virtual pci device");
    return -1;
}

static void make_addr_reg_from_config(uint32_t conf, vmm_pci_address_t *addr, uint8_t *reg) {
    addr->bus = (conf >> 16) & MASK(8);
    addr->dev = (conf >> 11) & MASK(5);
    addr->fun = (conf >> 8) & MASK(3);
    *reg = conf & MASK(8);
}

static vmm_pci_entry_t *find_device(vmm_pci_space_t *self, vmm_pci_address_t addr) {
    if (addr.bus != 0 || addr.dev >= 32 || addr.fun >= 8) {
        return NULL;
    }
    return self->bus0[addr.dev][addr.fun];
}

int vmm_pci_io_port_in(void *cookie, unsigned int port_no, unsigned int size, unsigned int *result) {
    vmm_pci_space_t *self = (vmm_pci_space_t*)cookie;
    uint8_t offset;

    if (port_no >= PCI_CONF_PORT_ADDR && port_no < PCI_CONF_PORT_ADDR_END) {
        offset = port_no - PCI_CONF_PORT_ADDR;
        assert(port_no + size <= PCI_CONF_PORT_ADDR_END);
        /* Emulate read addr. */
        *result = 0;
        memcpy(result, ((char*)&self->conf_port_addr) + offset, size);
        return 0;
    }
    assert(port_no >= PCI_CONF_PORT_DATA && port_no + size <= PCI_CONF_PORT_DATA_END);
    offset = port_no - PCI_CONF_PORT_DATA;

    /* construct a pci address from the current value in the config port */
    vmm_pci_address_t addr;
    uint8_t reg;
    make_addr_reg_from_config(self->conf_port_addr, &addr, &reg);
    reg += offset;

    /* Check if this device exists */
    vmm_pci_entry_t *dev = find_device(self, addr);
    if (!dev) {
        /* if the guest strayed from bus 0 then somethign went wrong. otherwise random reads
         * could just be it probing for devices */
        if (addr.bus != 0) {
            ZF_LOGI("Guest attempted access to non existent device %02x:%02x.%d register 0x%x", addr.bus, addr.dev, addr.fun, reg);
        } else {
            DPRINTF(3, "Ignoring guest probe for device %02x:%02x.%d register 0x%x\n", addr.bus, addr.dev, addr.fun, reg);
        }
        *result = -1;
        return 0;
    }
    int error = dev->ioread(dev->cookie, reg, size, result);
    if (error) {
        return error;
    }
    /* Strip out any multi function reporting */
    if (reg + size > PCI_HEADER_TYPE && reg <= PCI_HEADER_TYPE) {
        /* This read overlapped with the header type, work out where it is and mask
         * the MF bit out */
        int header_offset = PCI_HEADER_TYPE - reg;
        unsigned int mf_mask = ~(BIT(7) << (header_offset * 8));
        (*result) &= mf_mask;
    }
    return 0;
}

int vmm_pci_io_port_out(void *cookie, unsigned int port_no, unsigned int size, unsigned int value) {
    vmm_pci_space_t *self = (vmm_pci_space_t*)cookie;
    uint8_t offset;

    if (port_no >= PCI_CONF_PORT_ADDR && port_no < PCI_CONF_PORT_ADDR_END) {
        offset = port_no - PCI_CONF_PORT_ADDR;
        assert(port_no + size <= PCI_CONF_PORT_ADDR_END);
        /* Emulated set addr. First mask out the bottom two bits of the address that
         * should never be used*/
        value &= ~MASK(2);
        memcpy(((char*)&self->conf_port_addr) + offset, &value, size);
        return 0;
    }
    assert(port_no >= PCI_CONF_PORT_DATA && port_no + size <= PCI_CONF_PORT_DATA_END);
    offset = port_no - PCI_CONF_PORT_DATA;

    /* construct a pci address from the current value in the config port */
    vmm_pci_address_t addr;
    uint8_t reg;
    make_addr_reg_from_config(self->conf_port_addr, &addr, &reg);
    reg += offset;

    /* Check if this device exists */
    vmm_pci_entry_t *dev = find_device(self, addr);
    if (!dev) {
        ZF_LOGI("Guest attempted access to non existent device %02x:%02x.%d register 0x%x", addr.bus, addr.dev, addr.fun, reg);
        return 0;
    }
    return dev->iowrite(dev->cookie, reg + offset, size, value);
}

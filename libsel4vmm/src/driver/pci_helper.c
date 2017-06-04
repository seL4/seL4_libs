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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sel4/sel4.h>

#include <pci/virtual_pci.h>
#include <pci/helper.h>

#include "vmm/driver/pci_helper.h"

typedef struct pci_bar_emulation {
    vmm_pci_entry_t passthrough;
    int num_bars;
    vmm_pci_bar_t bars[6];
    uint32_t bar_writes[6];
} pci_bar_emulation_t;

typedef struct pci_irq_emulation {
    vmm_pci_entry_t passthrough;
    int irq;
} pci_irq_emulation_t;

typedef struct pci_passthrough_device {
    /* The address on the host system of this device */
    vmm_pci_address_t addr;
    /* Ops for accessing config space */
    vmm_pci_config_t config;
} pci_passthrough_device_t;

typedef struct pci_cap_emulation {
    vmm_pci_entry_t passthrough;
    int num_caps;
    uint8_t *caps;
    int num_ignore;
    uint8_t *ignore_start;
    uint8_t *ignore_end;
} pci_cap_emulation_t;

int vmm_pci_mem_device_read(void *cookie, int offset, int size, uint32_t *result) {
    if (offset < 0) {
        ZF_LOGE("Offset should not be negative");
        return -1;
    }
    if (offset + size >= 0x40) {
        ZF_LOGI("Indexing capability space not yet supported, returning 0");
        *result = 0;
        return 0;
    }
    *result = 0;
    memcpy(result, cookie + offset, size);
    return 0;
}

int vmm_pci_entry_ignore_write(void *cookie, int offset, int size, uint32_t value) {
    return 0;
}

void define_pci_host_bridge(vmm_pci_device_def_t *bridge) {
    *bridge = (vmm_pci_device_def_t) {
        .vendor_id = 0x5E14,
        .device_id = 0x42,
        .command = 0,
        .status = 0,
        .revision_id = 0x1,
        .prog_if = 0,
        .subclass = 0x0,
        .class_code = 0x06,
        .cache_line_size = 0,
        .latency_timer = 0,
        .header_type = 0x00,
        .bist = 0,
        .bar0 = 0,
        .bar1 = 0,
        .bar2 = 0,
        .bar3 = 0,
        .bar4 = 0,
        .bar5 = 0,
        .cardbus = 0,
        .subsystem_vendor_id = 0,
        .subsystem_id = 0,
        .expansion_rom = 0,
        .caps_pointer = 0,
        .reserved1 = 0,
        .reserved2 = 0,
        .reserved3 = 0,
        .interrupt_line = 0,
        .interrupt_pin = 0,
        .min_grant = 0,
        .max_latency = 0,
        .caps_len = 0,
        .caps = NULL
    };
}

static int passthrough_pci_config_ioread(void *cookie, int offset, int size, uint32_t *result) {
    pci_passthrough_device_t *dev = (pci_passthrough_device_t*)cookie;
    switch(size) {
    case 1:
        *result = dev->config.ioread8(dev->config.cookie, dev->addr, offset);
        break;
    case 2:
        *result = dev->config.ioread16(dev->config.cookie, dev->addr, offset);
        break;
    case 4:
        *result = dev->config.ioread32(dev->config.cookie, dev->addr, offset);
        break;
    default:
        assert(!"Invalid size");
    }
    return 0;
}

static int passthrough_pci_config_iowrite(void *cookie, int offset, int size, uint32_t val) {
    pci_passthrough_device_t *dev = (pci_passthrough_device_t*)cookie;
    switch(size) {
    case 1:
        dev->config.iowrite8(dev->config.cookie, dev->addr, offset, val);
        break;
    case 2:
        dev->config.iowrite16(dev->config.cookie, dev->addr, offset, val);
        break;
    case 4:
        dev->config.iowrite32(dev->config.cookie, dev->addr, offset, val);
        break;
    default:
        assert(!"Invalid size");
    }
    return 0;
}

static int pci_bar_emul_check_range(unsigned int offset, unsigned int size) {
    if (offset < PCI_BASE_ADDRESS_0 || offset + size > PCI_BASE_ADDRESS_5 + 4) {
        return 1;
    }
    return 0;
}

static uint32_t pci_make_bar(pci_bar_emulation_t *emul, int bar) {
    if (bar >= emul->num_bars) {
        return 0;
    }
    uint32_t raw = 0;
    raw |= emul->bars[bar].address;
    if (!emul->bars[bar].ismem) {
        raw |= 1;
    } else {
        if (emul->bars[bar].prefetchable) {
            raw |= BIT(3);
        }
    }
    raw |= (emul->bar_writes[bar] & ~MASK(emul->bars[bar].size_bits));
    return raw;
}

static int pci_irq_emul_read(void *cookie, int offset, int size, uint32_t *result) {
    pci_irq_emulation_t *emul = (pci_irq_emulation_t*)cookie;
    if(offset <= PCI_INTERRUPT_LINE && offset + size > PCI_INTERRUPT_LINE) {
        /* do the regular read, then patch in our value */
        int ret = emul->passthrough.ioread(emul->passthrough.cookie, offset, size, result);
        if (ret) {
            return ret;
        }
        int bit_offset = (PCI_INTERRUPT_LINE - offset) * 8;
        *result &= ~(MASK(8) << bit_offset);
        *result |= (emul->irq << bit_offset);
        return 0;
    } else {
        return emul->passthrough.ioread(emul->passthrough.cookie, offset, size, result);
    }
}

static int pci_irq_emul_write(void *cookie, int offset, int size, uint32_t value) {
    pci_irq_emulation_t *emul = (pci_irq_emulation_t*)cookie;
    if (offset == PCI_INTERRUPT_LINE && size == 1) {
        /* ignore */
        return 0;
    } else if(offset < PCI_INTERRUPT_LINE && offset + size >= PCI_INTERRUPT_LINE) {
        assert(!"Guest writing PCI configuration in an unsupported way");
        return -1;
    } else {
        return emul->passthrough.iowrite(emul->passthrough.cookie, offset, size, value);
    }
}

static int pci_bar_emul_read(void *cookie, int offset, int size, uint32_t *result) {
    pci_bar_emulation_t *emul = (pci_bar_emulation_t*)cookie;
    if (pci_bar_emul_check_range(offset, size)) {
        return emul->passthrough.ioread(emul->passthrough.cookie, offset, size, result);
    }
    /* Construct the bar value */
    int bar = (offset - PCI_BASE_ADDRESS_0) / 4;
    int bar_offset = offset & 3;
    uint32_t bar_raw = pci_make_bar(emul, bar);
    char *barp = (char*)&bar_raw;
    *result = 0;
    memcpy(result, barp + bar_offset, size);
    return 0;
}

static int pci_bar_emul_write(void *cookie, int offset, int size, uint32_t value) {
    pci_bar_emulation_t *emul = (pci_bar_emulation_t*)cookie;
    if (pci_bar_emul_check_range(offset, size)) {
        return emul->passthrough.iowrite(emul->passthrough.cookie, offset, size, value);
    }
    /* Construct the bar value */
    int bar = (offset - PCI_BASE_ADDRESS_0) / 4;
    int bar_offset = offset & 3;
    char *barp = (char*)&emul->bar_writes[bar];
    memcpy(barp + bar_offset, &value, size);
    return 0;
}

vmm_pci_entry_t vmm_pci_create_bar_emulation(vmm_pci_entry_t existing, int num_bars, vmm_pci_bar_t *bars) {
    pci_bar_emulation_t *bar_emul = malloc(sizeof(*bar_emul));
    assert(bar_emul);
    memcpy(bar_emul->bars, bars, sizeof(vmm_pci_bar_t) * num_bars);
    bar_emul->passthrough = existing;
    bar_emul->num_bars = num_bars;
    memset(bar_emul->bar_writes, 0, sizeof(bar_emul->bar_writes));
    return (vmm_pci_entry_t) {.cookie = bar_emul, .ioread = pci_bar_emul_read, .iowrite = pci_bar_emul_write};
}

vmm_pci_entry_t vmm_pci_create_irq_emulation(vmm_pci_entry_t existing, int irq) {
    pci_irq_emulation_t *irq_emul = malloc(sizeof(*irq_emul));
    assert(irq_emul);
    irq_emul->passthrough = existing;
    irq_emul->irq = irq;
    return (vmm_pci_entry_t) {.cookie = irq_emul, .ioread = pci_irq_emul_read, .iowrite = pci_irq_emul_write};
}

vmm_pci_entry_t vmm_pci_create_passthrough(vmm_pci_address_t addr, vmm_pci_config_t config) {
    pci_passthrough_device_t *dev = malloc(sizeof(*dev));
    assert(dev);
    dev->addr = addr;
    dev->config = config;
    ZF_LOGI("Creating passthrough device for %02x:%02x.%d", addr.bus, addr.dev, addr.fun);
    return (vmm_pci_entry_t){.cookie = dev, .ioread = passthrough_pci_config_ioread, .iowrite = passthrough_pci_config_iowrite};
}

int vmm_pci_helper_map_bars(vmm_t *vmm, libpci_device_iocfg_t *cfg, vmm_pci_bar_t *bars) {
    int i;
    int bar = 0;
    for (i = 0; i < 6; i++) {
        if (cfg->base_addr[i] == 0) {
            continue;
        }
        size_t size = cfg->base_addr_size[i];
        assert(size != 0);
        int size_bits = 31 - CLZ(size);
        if (BIT(size_bits) != size) {
            ZF_LOGE("PCI bar is not power of 2 size (%zu)", size);
            return -1;
        }
        bars[bar].size_bits = size_bits;
        if (cfg->base_addr_space[i] == PCI_BASE_ADDRESS_SPACE_MEMORY) {
            /* Need to map into the VMM. Make sure it is aligned */
            uintptr_t addr = vmm_map_guest_device(vmm, cfg->base_addr[i], size, BIT(size_bits));
            if(addr == 0) {
                ZF_LOGE("Failed to map PCI bar %p size %zu", (void*)(uintptr_t)cfg->base_addr[i], size);
                return -1;
            }
            bars[bar].ismem = 1;
            bars[bar].address = addr;
            bars[bar].prefetchable = cfg->base_addr_prefetchable[i];
        } else {
            /* Need to add the IO port range */
            int error = vmm_io_port_add_passthrough(&vmm->io_port, cfg->base_addr[i], cfg->base_addr[i] + size - 1, "PCI Passthrough Device");
            if (error) {
                return error;
            }
            bars[bar].ismem=0;
            bars[bar].address = cfg->base_addr[i];
            bars[bar].prefetchable = 0;
        }
        bar++;
    }
    return bar;
}

static int pci_cap_emul_read(void *cookie, int offset, int size, uint32_t *result) {
    pci_cap_emulation_t *emul = (pci_cap_emulation_t*)cookie;
    if(offset <= PCI_STATUS && offset + size > PCI_STATUS) {
        /* do the regular read, then patch in our value */
        int ret = emul->passthrough.ioread(emul->passthrough.cookie, offset, size, result);
        if (ret) {
            return ret;
        }
        int bit_offset = (PCI_STATUS - offset) * 8;
        *result &= ~(PCI_STATUS_CAP_LIST << bit_offset);
        if (emul->num_caps > 0) {
            *result |= (PCI_STATUS_CAP_LIST << bit_offset);
        }
        return 0;
    } else if(offset <= PCI_CAPABILITY_LIST && offset + size > PCI_CAPABILITY_LIST) {
        /* do the regular read, then patch in our value */
        int ret = emul->passthrough.ioread(emul->passthrough.cookie, offset, size, result);
        if (ret) {
            return ret;
        }
        int bit_offset = (PCI_CAPABILITY_LIST - offset) * 8;
        *result &= ~(MASK(8) << bit_offset);
        if (emul->num_caps > 0) {
            *result |= (emul->caps[0] << bit_offset);
        }
        return 0;
    }
    /* see if we are reading from any location that we would prefer not to */
    int i;
    for (i = 0; i < emul->num_ignore; i++) {
        if (offset <= emul->ignore_start[i] && offset+size > emul->ignore_end[i]) {
            /* who cares about the size, just ignore everything */
            ZF_LOGI("Attempted read at 0x%x of size %d from region 0x%x-0x%x", offset, size, emul->ignore_start[i], emul->ignore_end[i]);
            *result = 0;
            return 0;
        }
    }
    /* See if we are reading a capability index */
    for (i = 0; i < emul->num_caps; i++) {
        if (offset <= emul->caps[i] + 1 && offset + size > emul->caps[i] + 1) {
            /* do the regular read, then patch in our value */
            int ret = emul->passthrough.ioread(emul->passthrough.cookie, offset, size, result);
            if (ret) {
                return ret;
            }
            int bit_offset = (emul->caps[i] + 1 - offset) * 8;
            *result &= ~(MASK(8) << bit_offset);
            if (i + 1 < emul->num_caps) {
                *result |= (emul->caps[i + 1] << bit_offset);
            }
            return 0;
        }
    }
    /* Pass through whatever is left */
    return emul->passthrough.ioread(emul->passthrough.cookie, offset, size, result);
}

static int pci_cap_emul_write(void *cookie, int offset, int size, uint32_t value) {
    pci_cap_emulation_t *emul = (pci_cap_emulation_t*)cookie;
    /* Prevents writes to our ignored ranges. but let anything else through */
    int i;
    for (i = 0; i < emul->num_ignore; i++) {
        if (offset <= emul->ignore_start[i] && offset+size > emul->ignore_end[i]) {
            /* who cares about the size, just ignore everything */
            ZF_LOGI("Attempted write at 0x%x of size %d from region 0x%x-0x%x", offset, size, emul->ignore_start[i], emul->ignore_end[i]);
            return 0;
        }
    }
    return emul->passthrough.iowrite(emul->passthrough.cookie, offset, size, value);
}

vmm_pci_entry_t vmm_pci_create_cap_emulation(vmm_pci_entry_t existing, int num_caps, uint8_t *caps, int num_ranges, uint8_t *range_starts, uint8_t *range_ends) {
    pci_cap_emulation_t *emul = malloc(sizeof(*emul));
    emul->passthrough = existing;
    assert(emul);
    emul->num_caps = num_caps;
    emul->caps = malloc(sizeof(uint8_t) * num_caps);
    assert(emul->caps);
    memcpy(emul->caps, caps, sizeof(uint8_t) * num_caps);
    emul->num_ignore = num_ranges;
    emul->ignore_start = malloc(sizeof(uint8_t) * num_ranges);
    assert(emul->ignore_start);
    emul->ignore_end = malloc(sizeof(uint8_t) * num_ranges);
    assert(emul->ignore_end);
    memcpy(emul->ignore_start, range_starts, sizeof(uint8_t) * num_ranges);
    memcpy(emul->ignore_end, range_ends, sizeof(uint8_t) * num_ranges);
    return (vmm_pci_entry_t) {.cookie = emul, .ioread = pci_cap_emul_read, .iowrite = pci_cap_emul_write};
}

#define MAX_CAPS 256

vmm_pci_entry_t vmm_pci_no_msi_cap_emulation(vmm_pci_entry_t existing) {
    uint32_t value;
    int UNUSED error;
    /* Ensure this is a type 0 device */
    value = 0;
    error = existing.ioread(existing.cookie, PCI_HEADER_TYPE, 1, &value);
    assert(!error);
    assert( (value & (~BIT(7))) == PCI_HEADER_TYPE_NORMAL);
    /* Check if it has capability space */
    error = existing.ioread(existing.cookie, PCI_STATUS, 1, &value);
    assert(!error);
    if (! (value & PCI_STATUS_CAP_LIST)) {
        return existing;
    }
    /* First we need to scan the capability space, and detect any PCI caps
     * while we're at it */
    int num_caps;
    uint8_t caps[MAX_CAPS];
    int num_ignore;
    uint8_t ignore_start[2];
    uint8_t ignore_end[2];
    error = existing.ioread(existing.cookie, PCI_CAPABILITY_LIST, 1, &value);
    assert(!error);
    /* Mask off the bottom 2 bits, which are reserved */
    value &= ~MASK(2);
    num_caps = 0;
    num_ignore = 0;
    while (value != 0) {
        uint32_t cap_type = 0;
        error = existing.ioread(existing.cookie, value, 1, &cap_type);
        assert(!error);
        if (cap_type == PCI_CAP_ID_MSI) {
            assert(num_ignore < 2);
            ignore_start[num_ignore] = value;
            ignore_end[num_ignore] = value + 20;
            num_ignore++;
        } else if (cap_type == PCI_CAP_ID_MSIX) {
            ignore_start[num_ignore] = value;
            ignore_end[num_ignore] = value + 8;
            num_ignore++;
        } else {
            assert(num_caps < MAX_CAPS);
            caps[num_caps] = (uint8_t)value;
            num_caps++;
        }
        error = existing.ioread(existing.cookie, value + 1, 1, &value);
        assert(!error);
    }
    if (num_ignore > 0) {
        return vmm_pci_create_cap_emulation(existing, num_caps, caps, num_ignore, ignore_start, ignore_end);
    } else {
        return existing;
    }
}

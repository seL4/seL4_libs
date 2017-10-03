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

#include <autoconf.h>

#include <assert.h>
#include <sel4/sel4.h>
#include <stdlib.h>
#include <utils/util.h>
#include <vka/cspacepath_t.h>

/**
 * Request a cap to the IOPorts
 *
 * @param data cookie for the underlying implementation
 * @param start port number that a cap is needed to
 * @param end port number that a cap is needed to
 */
typedef seL4_CPtr (*arch_simple_get_IOPort_cap_fn)(void *data, uint16_t start_port, uint16_t end_port);

/**
 * Request a cap to a specific MSI IRQ number on the system
 *
 * @param irq the msi irq number to get the cap for
 * @param data cookie for the underlying implementation
 * @param the CNode in which to put this cap
 * @param the index within the CNode to put cap
 * @param Depth of index
 */
typedef seL4_Error (*arch_simple_get_msi_fn)(void *data, seL4_CNode root, seL4_Word index,
                                             uint8_t depth, seL4_Word pci_bus, seL4_Word pci_dev,
                                             seL4_Word pci_func, seL4_Word handle,
                                             seL4_Word vector);

typedef seL4_Error (*arch_simple_get_ioapic_fn)(void *data, seL4_CNode root, seL4_Word index,
                                               uint8_t depth, seL4_Word ioapic, seL4_Word pin,
                                               seL4_Word level, seL4_Word polarity,
                                               seL4_Word vector);

/**
 * Request a cap to a specific IRQ number on the system
 *
 * @param irq the irq number to get the cap for
 * @param data cookie for the underlying implementation
 * @param the CNode in which to put this cap
 * @param the index within the CNode to put cap
 * @param Depth of index
 */
typedef seL4_Error (*arch_simple_get_IRQ_handler_fn)(void *data, int irq, seL4_CNode cnode, seL4_Word index,
                                                     uint8_t depth);

#ifdef CONFIG_IOMMU
/**
 * Get the IO space capability for the specified PCI device and domain ID
 *
 * @param data cookie for the underlying implementation
 * @param domainID domain ID to request
 * @param deviceID PCI device ID
 * @param path Path to where to put this cap
 *
*/
typedef seL4_Error (*arch_simple_get_iospace_fn)(void *data, uint16_t domainID, uint16_t deviceID,
                                                 cspacepath_t *path);

#endif

typedef struct arch_simple {
    void *data;
    arch_simple_get_IOPort_cap_fn IOPort_cap;
#ifdef CONFIG_IOMMU
    arch_simple_get_iospace_fn iospace;
#endif
    arch_simple_get_msi_fn msi;
    arch_simple_get_IRQ_handler_fn irq;
    arch_simple_get_ioapic_fn ioapic;
} arch_simple_t;

static inline seL4_CPtr
arch_simple_get_IOPort_cap(arch_simple_t *arch_simple, uint16_t start_port, uint16_t end_port)
{
    if (!arch_simple) {
        ZF_LOGE("Arch-simple is NULL");
        return seL4_CapNull;
    }

    if (!arch_simple->IOPort_cap) {
        ZF_LOGE("%s not implemented", __FUNCTION__);
        return seL4_CapNull;
    }

    return arch_simple->IOPort_cap(arch_simple->data, start_port, end_port);
}

static inline seL4_Error
arch_simple_get_msi(arch_simple_t *arch_simple, cspacepath_t path, seL4_Word pci_bus,
                    seL4_Word pci_dev, seL4_Word pci_func, seL4_Word handle,
                    seL4_Word vector)
{
    if (!arch_simple) {
        ZF_LOGE("Arch-simple is NULL");
        return seL4_InvalidArgument;
    }

    if (!arch_simple->msi) {
        ZF_LOGE("%s not implemented", __FUNCTION__);
    }

    return arch_simple->msi(arch_simple->data, path.root, path.capPtr, path.capDepth, pci_bus,
                            pci_dev, pci_func, handle, vector);
}

static inline seL4_Error
arch_simple_get_ioapic(arch_simple_t *arch_simple, cspacepath_t path, seL4_Word ioapic,
                       seL4_Word pin, seL4_Word level, seL4_Word polarity,
                       seL4_Word vector)
{
    if (!arch_simple) {
        ZF_LOGE("Arch-simple is NULL");
        return seL4_InvalidArgument;
    }

    if (!arch_simple->ioapic) {
        ZF_LOGE("%s not implemented", __FUNCTION__);
    }

    return arch_simple->ioapic(arch_simple->data, path.root, path.capPtr, path.capDepth, ioapic,
                               pin, level, polarity, vector);
}

#ifdef CONFIG_IOMMU
static inline seL4_CPtr
arch_simple_get_iospace(arch_simple_t *arch_simple, uint16_t domainID, uint16_t deviceID, cspacepath_t *path)
{
    if (!arch_simple) {
        ZF_LOGE("Arch-simple is NULL");
        return seL4_CapNull;
    }
    if (!arch_simple->iospace) {
        ZF_LOGE("%s not implemented", __FUNCTION__);
        return seL4_CapNull;
    }

    return arch_simple->iospace(arch_simple->data, domainID, deviceID, path);
}
#endif


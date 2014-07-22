/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */
/* x86 VMM PCI Driver, which manages the host's PCI devices, and handles guest OS PCI config space
 * read & writes.
 *     Authors:
 *         Qian Ge
 *         Xi Ma Chen
 */

#ifndef __LIB_VMM_DRIVER_PCI_H__
#define __LIB_VMM_DRIVER_PCI_H__

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <sel4/sel4.h>
#include <pci/pci.h>

#include "vmm/config.h"
#include "vmm/helper.h"

#define LABEL_PCI_FIND_DEVICE 0x2f3d

static inline bool vmm_driver_pci_find_device(seL4_CPtr ep, uint16_t vendor_id, uint16_t device_id,
        libpci_device_t *out, int j) {
    assert(out);

    dprintf(4, "  sending IPC to LIB_VMM_DRIVER_PCI_CAP asking for device vid 0x%x did 0x%x\n",
            vendor_id, device_id);

    /* IPC the PCI driver to find info about this device. */
    seL4_MessageInfo_t msg = seL4_MessageInfo_new(LABEL_PCI_FIND_DEVICE, 0, 0, 3);
    seL4_SetMR(0, vendor_id);
    seL4_SetMR(1, device_id);
    seL4_SetMR(2, j);

    seL4_MessageInfo_t reply_msg = seL4_Call(ep, msg);

    int reply_len = seL4_MessageInfo_get_length(reply_msg);
    if (reply_len == 0) {
        /* Failed to find device. */
        return false;
    }
    /* Found device. */
    assert(reply_len == ROUND_UP(sizeof(libpci_device_t), 4) / 4);
    vmm_get_buf(0, (char*)out, sizeof(libpci_device_t));
    return true;
}

#endif /* __LIB_VMM_DRIVER_PCI_H__ */

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
 *         Adrian Danis
 *         Xi Ma Chen
 */

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <sel4/sel4.h>
#include <pci/pci.h>
#include <pci/virtual_pci.h>
#include <pci/helper.h>
#include <simple/simple.h>

#include "vmm/config.h"
#include "vmm/vmm.h"
#include "vmm/io.h"
#include "vmm/helper.h"
#include "vmm/platform/guest_devices.h"
#include "vmm/driver/pci.h"

static libpci_virtual_pci_t vmm_pci_gs[LIB_VMM_GUEST_NUMBER];
static bool vmm_pci_gs_initialised[LIB_VMM_GUEST_NUMBER];

/* Rebase a device for a Guest OS by using the PCI emulation feature in libpci, which taps into PCI
 * config space operations and pretends that the BIOS has mapped this device somewhere else. This
 * function is used to effectively lie to the guest OS about where it thinks the paddr of the
 * devices are, which should be the guest-paddr location we mapped the device into.
 */
static int vmm_pci_init_rebase_device(libpci_virtual_pci_t *vpci, vmm_guest_device_cfg_t *dv, int *device) {
    int i;
    assert(vpci && dv);
#if 1
    if (! (dv->mem_map && (dv->map_offset != 0x0UL)) ) {
        /* No need to rebase if device iomap is disabled, or there is no map_offset (1:1). */
        return 0;
    }
#endif

    /* Find device in scanned device list. */
    libpci_device_t *devices[36];
    int n = libpci_find_device_all(dv->ven, dv->dev, devices);
    if (n == 0) {
        printf(COLOUR_Y "WARNING: " COLOUR_RESET "could not find device vid 0x%x did 0x%x,"
               "device will be ignored.\n", dv->ven, dv->dev);
        return -1; /* Ignore device. */
    }
    for (i = 0; i < n; i++) {
        libpci_device_t *d = devices[i];

        libpci_device_iocfg_t *cfg = &d->cfg;

        /* Allow device in guest's virtual PCI. */
        libpci_vdevice_t *v = vpci->vdevice_assign(vpci);
        assert(v);
        assert(*device < 32);
        v->enable(v, 0, *device, 0, d);
        (*device)++;
//    v->enable(v, d->bus, d->dev, d->fun, d);
        v->allow_extended_pci_config_space = dv->pci_cfg_extended;

        /* Loop through each BAR and rebase it according to the map offset. */
        for (int i = 0; i < 6; i++) {
            if (cfg->base_addr[i] == 0 || cfg->base_addr_64H[i]) continue;
            if (cfg->base_addr_space[i] == PCI_BASE_ADDRESS_SPACE_IO) {
                /* Ignore any IO base addresses - we do not memory map them. */
                continue;
            }
            uint32_t paddr = libpci_device_iocfg_get_baseaddr32(cfg, i);
            uint32_t cvaddr = paddr + dv->map_offset;
            v->rebase_addr_realdevice(v, i, cvaddr, d);
            dprintf(2, "vpci :     %s : rebased 0x%x ---> 0x%x.\n", d->device_name, paddr, cvaddr);
        }
    }
    return 1;
}

/* Initialise the PCI guest structure. This is lazily called on the first PCI related instruction
 * from the guest.
 */
static void vmm_pci_init_guest(seL4_Word guest_badge) {
    int guest_id = VMM_GUEST_BADGE_TO_ID(guest_badge);
    assert(guest_id >= 0 && guest_id < LIB_VMM_GUEST_NUMBER);
    assert(!vmm_pci_gs_initialised[guest_id]);

    dprintf(2, "vpci: initialising guest %d virtual PCI...\n", guest_id);

    libpci_virtual_pci_t *vpci = &vmm_pci_gs[guest_id];
    libpci_virtual_pci_init(vpci);
    vpci->override_allow_all_devices = VMM_PCI_PASSTHROUGH_ALL_PCI_DEVICES;

    if (VMM_PCI_PASSTHROUGH_ALL_PCI_DEVICES) {
        vmm_pci_gs_initialised[guest_id] = true;
        return;
    }
    int device = 0;

    for (int i = 0; ; i++) {
        vmm_guest_device_cfg_t *dv = &vmm_plat_guest_allowed_devices[i];

        /* 0xFFFF vendor marks end of the allowed devices list. */
        if (dv->ven == 0xFFFF) {
            break;
        }

        /* An offsetted map may need virtual device address rebase feature to work. */
        int rebased = vmm_pci_init_rebase_device(vpci, dv, &device);
        switch(rebased) {
            case 1:
            case -1:
                break;
            case 0: {
                /* Allow this device on simple passthrough. */
                int ret = vpci->device_allow_id(vpci, dv->ven, dv->dev);
                assert(ret);
                }
                break;
        }
    }

    vmm_pci_gs_initialised[guest_id] = true;
}

/* Handle Guest PCI IO port request. */
static unsigned int vmm_pci_guest_msg_proc(seL4_Word _sender, unsigned int len) {
    io_msg_t recv_msg, send_msg;
    unsigned int ret = sizeof (send_msg);
    vmm_get_msg((void*)&recv_msg, len);
    send_msg.result = LIB_VMM_ERR;

    dprintf(4, "pci driver recv msg in %d, port 0x%x, size %d, from 0x%x\n",
            recv_msg.in, recv_msg.port_no, recv_msg.size, recv_msg.sender);

    /* Initialise guest virtual PCI state structure, if this is the first IOPort request. */
    int guest_id = VMM_GUEST_BADGE_TO_ID(recv_msg.sender);
    assert(guest_id >= 0 && guest_id < LIB_VMM_GUEST_NUMBER);
    if(!vmm_pci_gs_initialised[guest_id]) {
        vmm_pci_init_guest(recv_msg.sender);
        assert(vmm_pci_gs_initialised[guest_id]);
    }
    libpci_virtual_pci_t *vpci = &vmm_pci_gs[guest_id];

    if (recv_msg.in) /* Guest wants to read IO port. */
        send_msg.result = vpci->ioread(vpci, recv_msg.port_no, &send_msg.val, recv_msg.size);
    else /* Guest wants to write IO port. */
        send_msg.result = vpci->iowrite(vpci, recv_msg.port_no, recv_msg.val, recv_msg.size);

    dprintf(4, "pci driver reply msg ret %d val 0x%x\n", send_msg.result, send_msg.val);

    if (send_msg.result != 0)
        send_msg.result = LIB_VMM_ERR;

    vmm_put_msg((void *)&send_msg, sizeof (send_msg));
    return ret;
}

/* Handle VMM host thread find device call. This is used by the main VMM module to communicate
 * with the PCI manager module about discovered devices. */
static unsigned int vmm_pci_host_msg_proc(seL4_Word _sender, unsigned int len) {
    if (len != 3) {
        printf("WARNING: PCI host msg invalid format.\n");
        return 0;
    }

    uint16_t vendor_id = seL4_GetMR(0);
    uint16_t device_id = seL4_GetMR(1);
    int n = seL4_GetMR(2);
    libpci_device_t *devs[32];
    int devices = libpci_find_device_all(vendor_id, device_id, devs);
    if (n >= devices) {
       if (n == 0) {
            /* Could not find matching device. */
            dprintf(2, "PCI Driver :: could not find matching device vid 0x%x did 0x%x.\n",
                    vendor_id, device_id);
        }
        return 0;
    }

    /* Reply with found device info structure. 
     * NOTE: the pointers inside this structure will be invalid, as they will still point to
     * memory in the PCI manager driver module's virtual addr space. */
    return vmm_put_buf(0, (const char*) devs[n], sizeof(libpci_device_t));
}

seL4_CPtr libvmm_io_cap(void *data, uint16_t start_port, uint16_t end_port) {
    return LIB_VMM_IO_PCI_CAP;
}

/* The entry function for PCI manager driver module. */
void vmm_driver_pci_run(void) {

    seL4_MessageInfo_t msg;
    seL4_Word sender, label;
    unsigned int len, reply_len = 0; 
    seL4_MessageInfo_t reply;

    dprintf(2, "PCI DRIVER STARTED\n");

    /* construct a fake simple that just returns LIB_VMM_IO_PCI_CAP so
     * we can construct an I/O ops for libpci */
    simple_t io_simple = {.IOPort_cap = libvmm_io_cap};
    ps_io_port_ops_t ioops;
    sel4platsupport_get_io_port_ops(&ioops, &io_simple);

    /* Scan PCI devices on the host system. */
    libpci_scan(ioops);

    while (1) {
        /* Wait for incoming PCI driver msg. */
        msg = seL4_Wait(LIB_VMM_DRIVER_PCI_CAP, &sender);
        len = seL4_MessageInfo_get_length(msg);
        label = seL4_MessageInfo_get_label(msg);
        reply_len = 0;

        switch (label) {
            case LABEL_PCI_FIND_DEVICE:
                /* Handle VMM host thread find device call. */
                reply_len = vmm_pci_host_msg_proc(sender, len);
                break;
            case LABEL_VMM_GUEST_IO_MESSAGE:
                /* Handle Guest PCI IO port request. */
                reply_len = vmm_pci_guest_msg_proc(sender, len);
                break;
            default:
                printf("WARNING: PCI driver recieved unknown message.\n");
                assert(!"PCI driver recieved unknown message.");
                break;
        }

        reply = seL4_MessageInfo_new(0, 0, 0, reply_len);
        seL4_Reply(reply);
    }
}


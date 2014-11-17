/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/* Initialization functions related to the seL4 side of vmm
 * booting and management. */

#include <autoconf.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sel4/sel4.h>
#include <simple/simple.h>
#include <vka/capops.h>

#include "vmm/platform/boot.h"
#include "vmm/platform/guest_vspace.h"

/* The guest only has a single cap in its cnode (fault ep) so define
 * the smallest EP possible */
#define GUEST_CNODE_SIZE       2

/* This is the slot we place its fault endpoint in */
#define LIB_VMM_GUEST_OS_FAULT_EP_CAP     2

int vmm_init(vmm_t *vmm, simple_t simple, vka_t vka, vspace_t vspace, platform_callbacks_t callbacks) {
    int err;
    memset(vmm, 0, sizeof(vmm_t));
    vmm->vka = vka;
    vmm->host_simple = simple;
    vmm->host_vspace = vspace;
    vmm->plat_callbacks = callbacks;
    // Currently set this to 4k pages by default
    vmm->page_size = seL4_PageBits;
    err = vmm_pci_init(&vmm->pci);
    if (err) {
        return err;
    }
    err = vmm_io_port_init(&vmm->io_port);
    if (err) {
        return err;
    }
    return 0;
}

static int create_guest_fault_ep(vmm_t *vmm) {
    int error;
    vmm->guest_fault_ep = vka_alloc_endpoint_leaky(&vmm->vka);
    if (vmm->guest_fault_ep == 0) {
        return -1;
    }
    error = vka_cspace_alloc_path(&vmm->vka, &vmm->reply_slot);
    if (error) {
        return error;
    }
    return 0;
}

int vmm_init_host(vmm_t *vmm) {
    int error;
    error = create_guest_fault_ep(vmm);
    if (error) {
        return error;
    }
    vmm->done_host_init = 1;
    return 0;
}

static void install_guest_fault_ep(vmm_t *vmm) {
    cspacepath_t host_fault_path;
    cspacepath_t guest_fault_path;
    int error;

    vka_cspace_make_path(&vmm->vka, vmm->guest_fault_ep, &host_fault_path);

    guest_fault_path.root = vmm->guest_cnode;
    guest_fault_path.capPtr = LIB_VMM_GUEST_OS_FAULT_EP_CAP;
    guest_fault_path.capDepth = GUEST_CNODE_SIZE;

    error = vka_cnode_mint(&guest_fault_path, &host_fault_path, seL4_AllRights, seL4_CapData_Badge_new(LIB_VMM_GUEST_OS_FAULT_EP_BADGE));
    assert(error == seL4_NoError);
}

int vmm_init_guest(vmm_t *vmm, int priority) {
    assert(vmm->done_host_init);
    int error;
    /* allocate tcb, cnode and ept */
    vmm->guest_tcb = vka_alloc_tcb_leaky(&vmm->vka);
    if (vmm->guest_tcb == 0) {
        return -1;
    }
    vmm->guest_cnode = vka_alloc_cnode_object_leaky(&vmm->vka, GUEST_CNODE_SIZE);
    if (vmm->guest_cnode == 0) {
        return -1;
    }
    vmm->guest_pd = vka_alloc_ept_page_directory_pointer_table_leaky(&vmm->vka);
    if (vmm->guest_pd == 0) {
        return -1;
    }
    vmm->guest_vcpu = vka_alloc_vcpu_leaky(&vmm->vka);
    if (vmm->guest_vcpu == 0) {
        return -1;
    }
    /* Initialize a vspace for the guest */
    error = vmm_get_guest_vspace(&vmm->host_vspace, &vmm->host_vspace, &vmm->guest_mem.vspace, &vmm->vka, vmm->guest_pd);
    if (error) {
        return error;
    }

    /* Set the guest TCB information */
    install_guest_fault_ep(vmm);
    error = seL4_TCB_SetSpace(vmm->guest_tcb, LIB_VMM_GUEST_OS_FAULT_EP_CAP, vmm->guest_cnode,
                              seL4_CapData_Guard_new(0, 32 - GUEST_CNODE_SIZE),
                              vmm->guest_pd, seL4_NilData);
    assert(error == seL4_NoError);
    error = seL4_TCB_SetPriority(vmm->guest_tcb, priority);
    assert(error == seL4_NoError);
    error = seL4_IA32_VCPU_SetTCB(vmm->guest_vcpu, vmm->guest_tcb);
    assert(error == seL4_NoError);
    /* Init guest memory information. 
     * TODO: should probably done elsewhere */
    vmm->guest_mem.num_ram_regions = 0;
    vmm->guest_mem.ram_regions = malloc(0);
    vmm->done_guest_init = 1;
    return 0;
}

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
#include "vmm/processor/apicdef.h"
#include "vmm/processor/lapic.h"

int vmm_init(vmm_t *vmm, allocman_t *allocman, simple_t simple, vka_t vka, vspace_t vspace, platform_callbacks_t callbacks) {
    int err;
    memset(vmm, 0, sizeof(vmm_t));
    vmm->allocman = allocman;
    vmm->vka = vka;
    vmm->host_simple = simple;
    vmm->host_vspace = vspace;
    vmm->plat_callbacks = callbacks;
    vmm->tcb = simple_get_tcb(&simple);
    vmm->sc = simple_get_sc(&simple);
    vmm->sched_ctrl = simple_get_sched_ctrl(&simple, 0);
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
    err = vmm_mmio_init(&vmm->mmio_list);
    if (err) {
        return err;
    }

    vmm->vmcall_handlers = NULL;
    vmm->vmcall_num_handlers = 0;

    return 0;
}

int vmm_init_host(vmm_t *vmm) {
    vmm->done_host_init = 1;
    return 0;
}

static int vmm_init_vcpu(vmm_t *vmm, unsigned int vcpu_num, int priority) {
    int UNUSED error;
    assert(vcpu_num < vmm->num_vcpus);
    vmm_vcpu_t *vcpu = &vmm->vcpus[vcpu_num];

    /* sel4 vcpu (vmcs) */
    vcpu->guest_vcpu = vka_alloc_vcpu_leaky(&vmm->vka);
    if (vcpu->guest_vcpu == 0) {
        return -1;
    }

    /* bind the VCPU to the VMM thread */
    error = seL4_X86_VCPU_SetTCB(vcpu->guest_vcpu, vmm->tcb);
    assert(error == seL4_NoError);

    vcpu->vmm = vmm;
    vcpu->vcpu_id = vcpu_num;

    /* All LAPICs are created enabled, in virtual wire mode */
    vmm_create_lapic(vcpu, 1);

    return 0;
}

int vmm_init_guest(vmm_t *vmm, int priority) {
    return vmm_init_guest_multi(vmm, priority, 1);
}

int vmm_init_guest_multi(vmm_t *vmm, int priority, int num_vcpus) {
    int error;

    assert(vmm->done_host_init);

    vmm->num_vcpus = num_vcpus;
    vmm->vcpus = malloc(num_vcpus * sizeof(vmm_vcpu_t));
    if (!vmm->vcpus) {
        return -1;
    }

    /* Create an EPT which is the pd for all the vcpu tcbs */
    vmm->guest_pd = vka_alloc_ept_pml4_leaky(&vmm->vka);
    if (vmm->guest_pd == 0) {
        return -1;
    }
    /* Assign an ASID */
    error = simple_ASIDPool_assign(&vmm->host_simple, vmm->guest_pd);
    if (error != seL4_NoError) {
        ZF_LOGE("Failed to assign ASID pool to EPT root");
        return -1;
    }
    /* Install the guest PD */
    error = seL4_TCB_SetEPTRoot(vmm->tcb, vmm->guest_pd);
    assert(error == seL4_NoError);
    /* Initialize a vspace for the guest */
    error = vmm_get_guest_vspace(&vmm->host_vspace, &vmm->host_vspace,
            &vmm->guest_mem.vspace, &vmm->vka, vmm->guest_pd);
    if (error) {
        return error;
    }

    for (int i = 0; i < num_vcpus; i++) {
        vmm_init_vcpu(vmm, i, priority);
    }

    /* Init guest memory information.
     * TODO: should probably done elsewhere */
    vmm->guest_mem.num_ram_regions = 0;
    vmm->guest_mem.ram_regions = malloc(0);

    vmm_mmio_add_handler(&vmm->mmio_list, APIC_DEFAULT_PHYS_BASE,
            APIC_DEFAULT_PHYS_BASE + sizeof(struct local_apic_regs) - 1,
            NULL, "Local APIC", vmm_apic_mmio_read, vmm_apic_mmio_write);

    vmm->done_guest_init = 1;
    return 0;
}

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

/*This file contains structure definitions and function prototype related with VMM.*/

#pragma once

#include <sel4/sel4.h>

#include <vka/vka.h>
#include <simple/simple.h>
#include <vspace/vspace.h>
#include <allocman/allocman.h>

typedef struct vmm vmm_t;
typedef struct vmm_vcpu vmm_vcpu_t;

#include "vmm/platform/vmexit.h"
#include "vmm/driver/pci.h"
#include "vmm/io.h"
#include "vmm/platform/guest_memory.h"
#include "vmm/guest_state.h"
#include "vmm/vmexit.h"
#include "vmm/mmio.h"
#include "vmm/processor/lapic.h"
#include "vmm/vmcall.h"
#include "vmm/vmm_manager.h"

/* ID of the boot vcpu in a vmm */
#define BOOT_VCPU 0

/* System callbacks passed from the user to the library. These need to
 * be passed in as their definitions are invisible to this library */
typedef struct platform_callbacks {
    int (*get_interrupt)();
    int (*has_interrupt)();
    int (*do_async)(seL4_Word badge);
    seL4_CPtr (*get_async_event_notification)();
} platform_callbacks_t;

/* Stores informatoin about the guest image we are loading. This information probably stops
 * being relevant / useful after we start running. Most of this assumes
 * we are loading a guest kernel elf image and that we are acting as some kind of bootloader */
typedef struct guest_image {
    /* Alignment we used when loading the kernel image */
    size_t alignment;
    /* Entry point when the VM starts */
    uintptr_t entry;
    /* Base address (in guest physical) where the image was loaded */
    uintptr_t load_paddr;
    /* Base physical address the image was linked for */
    uintptr_t link_paddr;
    uintptr_t link_vaddr;
    /* If we are loading a guest elf then we may not have been able to put it where it
     * requested. This is the relocation offset */
    int relocation_offset;
    /* Guest physical address of where we built its page directory */
    uintptr_t pd;
    /* Guest physical address of where we stashed the kernel cmd line */
    uintptr_t cmd_line;
    size_t cmd_line_len;
    /* Guest physical address of where we created the boot information */
    uintptr_t boot_info;
    /* Boot module information */
    uintptr_t boot_module_paddr;
    size_t boot_module_size;
} guest_image_t;

/* Represents a libsel4vmm vcpu */
typedef struct vmm_vcpu {
    /* kernel objects */
    seL4_CPtr guest_vcpu;

    /* context */
    guest_state_t guest_state;

    /* parent vmm */
    vmm_t *vmm;

    vmm_lapic_t *lapic;
    int vcpu_id;

    /* is the vcpu online */
    int online;
} vmm_vcpu_t;

/* Represents a vmm instance that runs a single guest with one or more vcpus */
typedef struct vmm {
    /* Debugging state for sanity */
    int done_host_init;
    int done_guest_init;
    /* Allocators, vspaces, and other things for resource management */
    vka_t vka;
    simple_t host_simple;
    vspace_t host_vspace;
    /* due ot limitation of the vka interface we still need an explicit allocman */
    allocman_t *allocman;

    /* TCB of the VMM thread
     * TODO: Should eventually have one vmm thread per vcpu */
    seL4_CPtr tcb;
    seL4_CPtr sc;
    seL4_CPtr sched_ctrl;

    /* platform callback functions */
    platform_callbacks_t plat_callbacks;

    /* Default page size to use */
    int page_size;

    /* Memory related */
    guest_image_t guest_image;
    seL4_CPtr guest_pd;
    guest_memory_t guest_mem;

    /* Exit handling table */
    vmexit_handler_ptr vmexit_handlers[VMM_EXIT_REASON_NUM];
    /* PCI emulation state */
    vmm_pci_space_t pci;

    /* IO port management */
    vmm_io_port_list_t io_port;

    /* MMIO management. Should probably be per-vcpu, e.g. we can't handle the
     * guest trying to remap a local APIC */
    vmm_mmio_list_t mmio_list;

    unsigned int num_vcpus;
    vmm_vcpu_t *vcpus;

    vmcall_handler_t *vmcall_handlers;
    unsigned int vmcall_num_handlers;

    /*TODO add
        map of vcpu affinities
    */
} vmm_t;

/* Finalize the VM before running it */
int vmm_finalize(vmm_t *vmm);

/*running vmm moudle*/
void vmm_run(vmm_t *vmm);

/* TODO htf did these get here? lets refactor everything  */
void vmm_sync_guest_state(vmm_vcpu_t *vcpu);
void vmm_sync_guest_context(vmm_vcpu_t *vcpu);
void vmm_reply_vm_exit(vmm_vcpu_t *vcpu);

/* mint a badged copy of the vmm's async event notification cap */
seL4_CPtr vmm_create_async_event_notification_cap(vmm_t *vmm, seL4_Word badge);


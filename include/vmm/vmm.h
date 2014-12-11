/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/*This file contains structure definitions and function prototype related with VMM.*/

#ifndef _LIB_VMM_VMM_H_
#define _LIB_VMM_VMM_H_

#include <sel4/sel4.h>

#include <vka/vka.h>
#include <simple/simple.h>
#include <vspace/vspace.h>

#include "vmm/platform/vmexit.h"
#include "vmm/driver/pci.h"
#include "vmm/io.h"
#include "vmm/platform/guest_memory.h"
#include "vmm/guest_state.h"
#include "vmm/vmexit.h"

/* TODO: Use a badge and/or this constant should be defined in libsel4 */
#define LIB_VMM_VM_EXIT_MSG_LEN    21

/* The badge used for the fault endpoint */
#define LIB_VMM_GUEST_OS_FAULT_EP_BADGE   42

/* System callbacks passed from the user to the library. These need to
 * be passed in as their definitions are invisible to this library */
typedef struct platform_callbacks {
    int (*get_interrupt)();
    int (*has_interrupt)();
    int (*do_async)(seL4_Word badge);
    seL4_CPtr (*get_async_event_aep)();
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

typedef struct vmm {
    /* Debugging state for sanity */
    int done_host_init;
    int done_guest_init;
    /* Allocators, vspaces, and other things for resource management */
    vka_t vka;
    simple_t host_simple;
    vspace_t host_vspace;

    /* platform callback functions */
    platform_callbacks_t plat_callbacks;

    /* Default page size to use */
    int page_size;

    /* Endpoint that will be given to guest TCB for fault handling */
    seL4_CPtr guest_fault_ep;

    /* Reply slot for guest messages */
    cspacepath_t reply_slot;

    /* Guest objects */
    seL4_CPtr guest_tcb;
    seL4_CPtr guest_cnode;
    seL4_CPtr guest_pd;
    seL4_CPtr guest_vcpu;

    /* Guest memory information and management */
    guest_memory_t guest_mem;
    guest_image_t guest_image;

    /* Exit handling table */
    vmexit_handler_ptr vmexit_handlers[VMM_EXIT_REASON_NUM];

    /* Guest emulation state */
    guest_state_t guest_state;

    /* PCI emulation state */
    vmm_pci_space_t pci;

    /* IO port management */
    vmm_io_port_list_t io_port;
} vmm_t;

/* Finalize the VM before running it */
int vmm_finalize(vmm_t *vmm);

/*running vmm moudle*/
void vmm_run(vmm_t *vmm);

/* TODO: Move to somewhere fucking sensible */
void vmm_have_pending_interrupt(vmm_t *vmm);

#endif

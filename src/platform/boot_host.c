/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/* Booting and setup functions used for host threads only.
 *
 *     Authors:
 *         Qian Ge
 *
 *     Fri 22 Nov 2013 05:15:37 EST
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "vmm/platform/boot.h"

/* ---------------------------------------------------------------------------------------------- */

/* Copy the endpoints into host thread for communication. */
static void vmm_plat_host_create_net(thread_rec_t *host) {
    /* Serial endpoint cap. */
    vmm_plat_mint_cap(LIB_VMM_DRIVER_SERIAL_CAP, vmm_plat_net.serial_ep, host);
    
    /* PCI endpoint cap. */
    vmm_plat_mint_cap(LIB_VMM_DRIVER_PCI_CAP, vmm_plat_net.pci_ep, host);

    /* Interrupt endpoint cap. */
    vmm_plat_mint_cap(LIB_VMM_DRIVER_INTERRUPT_CAP, vmm_plat_net.interrupt_ep, host);

    /* Fault endpoint cap. */
    vmm_plat_mint_cap(LIB_VMM_FAULT_EP_CAP, vmm_plat_net.fault_ep, host);
}

/* Set up seL4-specific things such as IPC buffer. Used for host threads only.
   Assumes the vspace has already been set up, and the ELF file has been loaded.
 */
void vmm_plat_host_setup_vspace(thread_rec_t *resource) {
    int ret = -1; 
    seL4_CPtr frame = 0;
    void *map_addr = NULL;
    
    assert(resource);
    if (resource->flag == LIB_VMM_GUEST_OS_THREAD) {
        panic("vmm_plat_host_setup_vspace should not be called for guest threads");
        return;
    }

    /* Allocate a frame to use as the VMM host thread's IPC buffer. */
    frame = vka_alloc_frame_leaky(&vmm_plat_vka, seL4_PageBits);
    assert(frame);

    /* Put host thread IPC buffer one page above its stack. */
    resource->ipc_buffer = resource->stack_top + (1 << seL4_PageBits);
    reservation_t *reserve_area = vspace_reserve_range_at(&resource->vspace,
            (void *)resource->ipc_buffer, (1 << seL4_PageBits), seL4_AllRights, 1);

    /* Map the IPC buffer into thread's vspace. */
    ret = vspace_map_pages_at_vaddr(&resource->vspace, &frame, (void *)resource->ipc_buffer, 1,
                                    seL4_PageBits, reserve_area); 
    assert(ret == seL4_NoError);

    resource->ipc_cap = frame;
    dprintf(4, "thread ipc buffer 0x%x\n", resource->ipc_buffer);

    /* Bind the IPC buffer with the thread's TCB configuration. */
    ret = seL4_TCB_SetIPCBuffer(resource->tcb, resource->ipc_buffer, resource->ipc_cap);
    assert(ret == seL4_NoError);

    /* Map the stack area. */
    map_addr = (void *) resource->stack_top - (1 << seL4_PageBits);
    for (int i = 0; i < LIB_VMM_STACK_PAGES; i++) {
        /* Allocate a frame for the host thread's stack. */
        frame = vka_alloc_frame_leaky(&vmm_plat_vka, seL4_PageBits);
        assert(frame);

        /* Map this stack frame into host thread space. */
        reserve_area = vspace_reserve_range_at(&resource->vspace, map_addr,
                (1 << seL4_PageBits), seL4_AllRights, 1); 
        ret = vspace_map_pages_at_vaddr(&resource->vspace, &frame, map_addr, 1, seL4_PageBits,
                                        reserve_area);
        assert(ret == seL4_NoError);

        map_addr -= (1 << seL4_PageBits);
    }
}

/* ---------------------------------------------------------------------------------------------- */

/* Launch a host thread. */
void vmm_plat_run_host_thread(thread_rec_t *resource, vmm_driver_seq no) {

    /* Init the thread context. */
    seL4_UserContext context; 
    seL4_TCB_ReadRegisters(resource->tcb, true, 0, 13, &context);

    context.eax = 0;
    context.ebx = no;
    context.ecx = 0;
    context.edx = 0;

    /* Entry point. */
    context.eip = resource->elf_entry;
    /* Top of stack. */
    context.esp = resource->stack_top;

    context.gs = IPCBUF_GDT_SELECTOR;
    
    vmm_plat_print_thread_context(resource->thread_name, &context);

    seL4_TCB_WriteRegisters(resource->tcb, true, 0, 13, &context);
}


/* Setup and boot a VMM host thread. */
void vmm_plat_init_host_thread(thread_rec_t *resource, unsigned int flag, char *elf_name,
                               seL4_Word badge_no, char *thread_name) {
                              
    unsigned int n_areas;
    guest_mem_area_t *areas;

    assert(resource);
    assert(elf_name);
    assert(thread_name);

    /* Create the basic kernel objects needed for thread to run. */
    vmm_plat_create_thread(resource, 0, flag, elf_name, badge_no, thread_name);

    /* Load regions from parsing the ELF file. */
    vmm_plat_elf_parse(elf_name, &n_areas, &areas);

    /* Set up the mem mapping and load elf file. */
    vmm_plat_create_thread_vspace(resource, n_areas, areas);

    /* Create the ep network to this thread. */
    vmm_plat_host_create_net(resource);
}




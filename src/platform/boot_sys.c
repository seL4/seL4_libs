/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */
/* External interfaces for booting the VMM system.
 *     Authors:
 *         Qian Ge
 */

#include <assert.h>
#include <stdlib.h>

#include <sel4/sel4.h>
#include <vka/capops.h>
#include <simple/simple.h>
#include <simple-stable/simple-stable.h>

#include "vmm/vmm.h"
#include "vmm/config.h"
#include "vmm/io.h"
#include "vmm/platform/boot.h"
#include "vmm/platform/sys.h"



extern vka_t vmm_plat_vka;
extern simple_t vmm_plat_simple;

/* Badge allocation; in this system, the badges are like PIDs.
 * All the endpoints belonging to a thread should be badged with the same number.
 * Threads in the system use these badges to identify the IPC msg sender. */
seL4_Word vmm_vmm_badge_next = LIB_VMM_VMM_BADGE_START;
seL4_Word vmm_guest_os_badge_next = LIB_VMM_GUEST_OS_BADGE_START;


/* Initalise the PCI thread and IO cap. */
void vmm_plat_boot_pci(thread_rec_t *thread) {

    int ret;
    cspacepath_t cap_src, cap_dest;
    seL4_CapData_t badge; 
    badge.words[0] = (X86_IO_PCI_CONFIG_START << 16) | X86_IO_PCI_CONFIG_END;

    vmm_plat_init_host_thread(thread, LIB_VMM_DRIVER_THREAD, LIB_VMM_DRIVER_IMAGE, LIB_VMM_DRIVER_PCI_BADGE, "pci");

    /* Init the IO cap*/
    /* for now, give the vcpu access to all io ports */
    vka_cspace_make_path(&vmm_plat_vka, 
        simple_get_IOPort_cap(&vmm_plat_simple, 0, /* all 2^16 io ports */(1<<16)-1), &cap_src);

    /* Construct path of ep cap in thread's cspace. */
    cap_dest.root = thread->cnode;
    cap_dest.capPtr = LIB_VMM_IO_PCI_CAP;
    cap_dest.capDepth = LIB_VMM_CNODE_SIZE;

    ret = vka_cnode_mint(&cap_dest, &cap_src, seL4_CanWrite | seL4_CanRead, badge);
    assert(ret == seL4_NoError); 

    /* Run the thread. */
    vmm_plat_run_host_thread(thread, pci_seq);
}

/* Initalise the thread that managing interrupts. */
void vmm_plat_boot_interrupt_manager(thread_rec_t *thread, thread_rec_t *host) {
    int i;
    int error;
    int ret;
    seL4_CapData_t badge; 
    cspacepath_t cap_src, cap_dest;

    /*FIXME: The interrupt driver thread takes control of the interrupt distribution, registers the
     * default EP for interrupt asynchronous msg. */
    vmm_plat_init_host_thread(thread, LIB_VMM_DRIVER_THREAD,
            LIB_VMM_DRIVER_IMAGE, LIB_VMM_DRIVER_INTERRUPT_BADGE, "interrupt_manager");

    /* Bind and give all the PIC interrupts to driver. */
    seL4_CPtr async_ep = vka_alloc_async_endpoint_leaky(&vmm_plat_vka);
    cspacepath_t async_path;
    assert(async_ep);
    vka_cspace_make_path(&vmm_plat_vka, async_ep, &async_path);

    error = seL4_TCB_BindAEP(thread->tcb, async_ep);
    assert(error == seL4_NoError);

    bool irq_badge_skip = false;
    for (i = 0; i < 16; i++) {
        if (i == 2) {
            /* IRQ 2 is reserved. */
            continue;
        }

        /* Allocate caps and get the corresponding i-th IRq controller. */
        cspacepath_t badged_ep;
        cspacepath_t irq_path;
        error = vka_cspace_alloc_path(&vmm_plat_vka, &irq_path);
        assert(error == seL4_NoError);
        error = vka_cspace_alloc_path(&vmm_plat_vka, &badged_ep);
        assert(error == seL4_NoError);
        error = simple_get_IRQ_control(&vmm_plat_simple, i, irq_path);
        assert(error == seL4_NoError);
        
        /* Construct a badge that will not collide with the async notification bit. */
        uint32_t irq_badge = BIT(i);
        if (irq_badge == LIB_VMM_DRIVER_ASYNC_MESSAGE_BADGE) {
            irq_badge_skip = true;
        }
        if (irq_badge_skip) {
            irq_badge <<= 1;
        }
        irq_badge |= LIB_VMM_DRIVER_ASYNC_MESSAGE_BADGE;
        dprintf(4, "IRQ %d is set to badge 0x%x.\n", i, irq_badge);

        /* Min a badged cap from IRQ controller with interrupt driver badge. */
        error = vka_cnode_mint(&badged_ep, &async_path, seL4_AllRights, seL4_CapData_Badge_new(irq_badge));
        assert(error == seL4_NoError);
        error = seL4_IRQHandler_SetEndpoint(irq_path.capPtr, badged_ep.capPtr);
        assert(error == seL4_NoError);
        /* Pre-ack the interrupt controller to clear it. */
        error = seL4_IRQHandler_Ack(irq_path.capPtr);
        assert(error == seL4_NoError);
 
        /* Now to give the IRQ handler to the interrupt manager. */
        vmm_plat_mint_cap(LIB_VMM_DRIVER_INTERRUPT_HANDLER_START + i, irq_path.capPtr, thread);
    }

    /* Give it the async endpoint cap. */
    error = seL4_CNode_Mint(thread->cnode, LIB_VMM_HOST_ASYNC_MESSAGE_CAP, LIB_VMM_CNODE_SIZE,
                            simple_get_cnode(&vmm_plat_simple), host->async_vmm_ep, 32,
                            seL4_AllRights, seL4_CapData_Badge_new(LIB_VMM_DRIVER_ASYNC_MESSAGE_BADGE));
    assert(error == seL4_NoError);

    /* Init the IO cap*/
    vka_cspace_make_path(&vmm_plat_vka, simple_get_IOPort_cap(&vmm_plat_simple, 0, 0), &cap_src);

    /* Construct path of ep cap in thread's cspace. */
    cap_dest.root = thread->cnode;
    cap_dest.capPtr = LIB_VMM_IO_PCI_CAP;
    cap_dest.capDepth = LIB_VMM_CNODE_SIZE;

    ret = vka_cnode_mint(&cap_dest, &cap_src, seL4_CanWrite | seL4_CanRead, badge);
    assert(ret == seL4_NoError); 

    /* Run the thread. */
    vmm_plat_run_host_thread(thread, interrupt_manager_seq);
}


/* Initialise the serial thread. */
void vmm_plat_boot_serial(thread_rec_t *thread) {

    int ret;
    cspacepath_t cap_src, cap_dest;
    seL4_CapData_t badge; 
    badge.words[0] = (X86_IO_SERIAL_1_START << 16) | X86_IO_SERIAL_1_END;

    vmm_plat_init_host_thread(thread, LIB_VMM_DRIVER_THREAD, LIB_VMM_DRIVER_IMAGE, LIB_VMM_DRIVER_SERIAL_BADGE, "serial");

    /* Init the IO cap. */
    vka_cspace_make_path(&vmm_plat_vka, simple_get_IOPort_cap(&vmm_plat_simple, 0, 0), &cap_src);

    /* Construct path of EP cap in thread's cspace. */
    cap_dest.root = thread->cnode;
    cap_dest.capPtr = LIB_VMM_IO_SERIAL_CAP;
    cap_dest.capDepth = LIB_VMM_CNODE_SIZE;

    ret = vka_cnode_mint(&cap_dest, &cap_src, seL4_CanWrite | seL4_CanRead, badge);
    assert(ret == seL4_NoError); 

    /* Run the thread. */
    vmm_plat_run_host_thread(thread, serial_seq);
}


/* Initialise a VMM host thread, then run it. */
void vmm_plat_start_vmm(thread_rec_t *thread) {
    vmm_plat_init_host_thread(thread, LIB_VMM_VMM_THREAD, LIB_VMM_DRIVER_IMAGE, vmm_vmm_badge_next++, "vmm");
    vmm_plat_run_host_thread(thread, vmm_seq);
}

/* Initialise a VMM guest thread, then run it. */
void vmm_plat_start_guest_os(thread_rec_t *thread, char *kernel_image, char *initrd_image,
                             thread_rec_t *host) {
    vmm_plat_init_guest_thread(thread, kernel_image, initrd_image, host, vmm_guest_os_badge_next++);
    vmm_plat_run_guest_thread(thread);
}


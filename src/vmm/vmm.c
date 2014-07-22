/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/* Functions for VMM main host thread.
 *
 *     Authors:
 *         Qian Ge
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sel4/sel4.h>

#include "vmm/config.h"
#include "vmm/vmm.h"
#include "vmm/vmexit.h"
#include "vmm/vmcs.h"

/* The guest control block that belongs to this vmm. */
gcb_t vmm_gc;

static vmexit_handler_ptr vmexit_handlers[VMM_EXIT_REASON_NUM];

/*reply vm exit exception, guest OS is resumed*/
static void vmm_reply_vm_exit(void) {

    seL4_SetMR(0, vmm_gc.context.eip);
    seL4_SetMR(1, vmm_gc.context.esp);
    seL4_SetMR(2, vmm_gc.context.eflags);
    seL4_SetMR(3, vmm_gc.context.eax);
    seL4_SetMR(4, vmm_gc.context.ebx);
    seL4_SetMR(5, vmm_gc.context.ecx);
    seL4_SetMR(6, vmm_gc.context.edx);
    seL4_SetMR(7, vmm_gc.context.esi);
    seL4_SetMR(8, vmm_gc.context.edi);
    seL4_SetMR(9, vmm_gc.context.ebp);
    seL4_SetMR(10, vmm_gc.context.tls_base);
    seL4_SetMR(11, vmm_gc.context.fs);
    seL4_SetMR(12, vmm_gc.context.gs);

    seL4_MessageInfo_t msg = seL4_MessageInfo_new(0, 0, 0, 13);

    seL4_Reply(msg);

}


/* Handle VM exit in VMM module. */
static void vmm_handle_vm_exit(seL4_Word sender) {

    /* Save the guest context. */
    unsigned int reason = seL4_GetMR(0);
    vmm_gc.reason = seL4_GetMR(0);
    vmm_gc.qualification = seL4_GetMR(1);
    vmm_gc.instruction_length = seL4_GetMR(2);
    vmm_gc.guest_physical = seL4_GetMR(3);
    vmm_gc.rflags = seL4_GetMR(4);
    vmm_gc.guest_interruptibility = seL4_GetMR(5);
    vmm_gc.control_entry = seL4_GetMR(6);
    vmm_gc.cr3 = seL4_GetMR(7);
    vmm_gc.sender = sender;

    vmm_gc.context.eip = seL4_GetMR(8);
    vmm_gc.context.esp = seL4_GetMR(9);
    vmm_gc.context.eflags = seL4_GetMR(10);
    vmm_gc.context.eax = seL4_GetMR(11);
    vmm_gc.context.ebx = seL4_GetMR(12);
    vmm_gc.context.ecx = seL4_GetMR(13);
    vmm_gc.context.edx = seL4_GetMR(14);
    vmm_gc.context.esi = seL4_GetMR(15);
    vmm_gc.context.edi = seL4_GetMR(16);
    vmm_gc.context.ebp = seL4_GetMR(17);
    vmm_gc.context.tls_base = seL4_GetMR(18);
    vmm_gc.context.fs = seL4_GetMR(19);
    vmm_gc.context.gs = seL4_GetMR(20);

    vmm_gc.cr0 = vmm_vmcs_read(LIB_VMM_VCPU_CAP, VMX_GUEST_CR0);
    vmm_gc.cr4 = vmm_vmcs_read(LIB_VMM_VCPU_CAP, VMX_GUEST_CR4);

    /* Distribute the task according to the exit info. */
    vmm_print_guest_context(3);

    if (!vmexit_handlers[reason]) {
        printf("VM_FATAL_ERROR ::: vm exit handler is NULL for reason 0x%x.\n", reason);
        vmm_print_guest_context(0);
        return;
    }

    /* Call the handler. */
    if (vmexit_handlers[reason](&vmm_gc) != LIB_VMM_SUCC) {
        printf("VM_FATAL_ERROR ::: vmexit handler return error\n");
        vmm_print_guest_context(0);
        return;
    }

    /* Reply to the VM exit exception to resume guest. */
    vmm_reply_vm_exit();
}



/* Entry point of of VMM main host module. */
void vmm_run(void) {
    seL4_MessageInfo_t msg;
    seL4_Word sender;
    seL4_Word msg_len, msg_label; 
    int halted = 0;

    dprintf(2, "VMM MAIN HOST MODULE STARTED\n");

    /* Init value for managing guest OS. */
    vmm_gc.cr0_mask = VMM_VMCS_CR0_MASK;
    vmm_gc.cr0_shadow = VMM_VMCS_CR0_SHADOW;
    vmm_gc.cr0 = VMM_VMCS_CR0_SHADOW;

    vmm_gc.cr4_mask = VMM_VMCS_CR4_MASK;
    vmm_gc.cr4_shadow = VMM_VMCS_CR4_SHADOW;
    vmm_gc.cr4 = VMM_VMCS_CR4_SHADOW;
    vmm_gc.interrupt_halt = 0;

    while (1) {
        /* Block and wait for incoming msg or VM exits. */
        msg = seL4_Wait(LIB_VMM_GUEST_OS_FAULT_EP_CAP, &sender);
        msg_len = seL4_MessageInfo_get_length(msg);
        msg_label = seL4_MessageInfo_ptr_get_label(&msg);

        dprintf(3, "VMM: receive msg from %d len %d label %d\n", sender, msg_len, msg_label);
        (void) msg_label;

        vmm_print_ipc_msg(msg_len);

        if (sender & LIB_VMM_DRIVER_ASYNC_MESSAGE_BADGE) {
            /* Received some kind of async notification. Currently assume
             * this means pending interrupt from the interrupt handler.
             * Assume interrupts are only for guest 0. TODO: proper support for interrupt
             * distribution to multiple guests. */
            vmm_gc.sender = LIB_VMM_GUEST_OS_BADGE_START;
            vmm_have_pending_interrupt(&vmm_gc);

        } else if (seL4_MessageInfo_get_length(msg) == LIB_VMM_VM_EXIT_MSG_LEN) {
            /* Received a guest OS VM Exit message from the seL4 kernel. */
            vmm_handle_vm_exit(sender);
        } else {
            panic("VMM main host module :: unknown message recieved.\n");
        }
        int new_halted = vmm_gc.interrupt_halt;
        if (!halted && new_halted) {
            /* suspend */
            seL4_TCB_Suspend(LIB_VMM_GUEST_TCB_CAP);
        }
        if (halted && !new_halted) {
            /* resume */
            seL4_TCB_Resume(LIB_VMM_GUEST_TCB_CAP);
        }
        halted = new_halted;

        dprintf(5, "VMM main host blocking for another message...\n");
    }

}


/* Initialise VMM library. */
void vmm_init(void) {
    /* Connect VM exit handlers to correct function pointers */
    memset(vmexit_handlers, 0, sizeof (vmexit_handler_ptr) * VMM_EXIT_REASON_NUM);
    vmexit_handlers[EXIT_REASON_PENDING_INTERRUPT] = vmm_pending_interrupt_handler;
    vmexit_handlers[EXIT_REASON_EXCEPTION_NMI] = vmm_exception_handler;
    vmexit_handlers[EXIT_REASON_CPUID] = vmm_cpuid_handler;
    vmexit_handlers[EXIT_REASON_MSR_READ] = vmm_rdmsr_handler;
    vmexit_handlers[EXIT_REASON_MSR_WRITE] = vmm_wrmsr_handler;
    vmexit_handlers[EXIT_REASON_EPT_VIOLATION] = vmm_ept_violation_handler;
    vmexit_handlers[EXIT_REASON_CR_ACCESS] = vmm_cr_access_handler;
    vmexit_handlers[EXIT_REASON_IO_INSTRUCTION] = vmm_io_instruction_handler;
    vmexit_handlers[EXIT_REASON_RDTSC] = vmm_rdtsc_instruction_handler;
    vmexit_handlers[EXIT_REASON_HLT] = vmm_hlt_handler;
}

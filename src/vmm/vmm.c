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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sel4/sel4.h>
#include <vka/capops.h>

#include "vmm/debug.h"
#include "vmm/vmm.h"
#include "vmm/interrupt.h"
#include "vmm/platform/boot_guest.h"

void vmm_sync_guest_context(vmm_vcpu_t *vcpu) {
    if (IS_MACHINE_STATE_MODIFIED(vcpu->guest_state.machine.context)) {
        seL4_UserContext context;
        context.eip = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EIP);
        context.esp = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_ESP);
        context.eflags = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EFLAGS);
        context.eax = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EAX);
        context.ebx = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EBX);
        context.ecx = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_ECX);
        context.edx = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EDX);
        context.esi = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_ESI);
        context.edi = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EDI);
        context.ebp = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EBP);
        context.tls_base = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_TLS_BASE);
        context.fs = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_FS);
        context.gs = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_GS);
        seL4_TCB_WriteRegisters(vcpu->guest_tcb, false, 0, 13, &context);
        /* Sync our context */
        MACHINE_STATE_SYNC(vcpu->guest_state.machine.context);
    }
}

static void vmm_reply_vm_exit(vmm_vcpu_t *vcpu) {
    assert(vcpu->guest_state.exit.in_exit);
    int msg_len = 0;

    if (IS_MACHINE_STATE_MODIFIED(vcpu->guest_state.machine.context)) {
        seL4_SetMR(0, vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EIP));
        seL4_SetMR(1, vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_ESP));
        seL4_SetMR(2, vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EFLAGS));
        seL4_SetMR(3, vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EAX));
        seL4_SetMR(4, vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EBX));
        seL4_SetMR(5, vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_ECX));
        seL4_SetMR(6, vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EDX));
        seL4_SetMR(7, vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_ESI));
        seL4_SetMR(8, vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EDI));
        seL4_SetMR(9, vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EBP));
        seL4_SetMR(10, vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_TLS_BASE));
        seL4_SetMR(11, vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_FS));
        seL4_SetMR(12, vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_GS));
        /* Sync our context */
        MACHINE_STATE_SYNC(vcpu->guest_state.machine.context);
        msg_len = 13;
    }

    seL4_MessageInfo_t msg = seL4_MessageInfo_new(0, 0, 0, msg_len);

    /* Before we resume the guest, ensure there is no dirty state around */
    assert(vmm_guest_state_no_modified(&vcpu->guest_state));
    vmm_guest_state_invalidate_all(&vcpu->guest_state);

    seL4_Send(vcpu->reply_slot.capPtr, msg);

    vcpu->guest_state.exit.in_exit = 0;
}

void vmm_sync_guest_state(vmm_vcpu_t *vcpu) {
    vmm_guest_state_sync_control_entry(&vcpu->guest_state, vcpu->guest_vcpu);
    vmm_guest_state_sync_control_ppc(&vcpu->guest_state, vcpu->guest_vcpu);
    vmm_guest_state_sync_cr0(&vcpu->guest_state, vcpu->guest_vcpu);
    vmm_guest_state_sync_cr3(&vcpu->guest_state, vcpu->guest_vcpu);
    vmm_guest_state_sync_cr4(&vcpu->guest_state, vcpu->guest_vcpu);

    if (vcpu->guest_state.exit.in_exit && !vcpu->guest_state.virt.interrupt_halt) {
        /* Guest is blocked, but we are no longer halted. Reply to it */
        vmm_reply_vm_exit(vcpu);
    }
}


/* Handle VM exit in VMM module. */
static void vmm_handle_vm_exit(vmm_vcpu_t *vcpu) {
    int reason = vmm_guest_exit_get_reason(&vcpu->guest_state);

    /* Distribute the task according to the exit info. */
    vmm_print_guest_context(3, vcpu);

    if (!vcpu->vmm->vmexit_handlers[reason]) {
        printf("VM_FATAL_ERROR ::: vm exit handler is NULL for reason 0x%x.\n", reason);
        vmm_print_guest_context(0, vcpu);
        return;
    }

    /* Call the handler. */
    if (vcpu->vmm->vmexit_handlers[reason](vcpu)) {
        printf("VM_FATAL_ERROR ::: vmexit handler return error\n");
        vmm_print_guest_context(0, vcpu);
        return;
    }

    /* Check for any interrupts.
       TODO: do this more sensibly once the interrupt controller
       emulation is part of the vmm */
    vmm_have_pending_interrupt(vcpu);

    /* Reply to the VM exit exception to resume guest. */
    vmm_sync_guest_state(vcpu);
}

static void vmm_update_guest_state_from_fault(vmm_vcpu_t *vcpu, seL4_Word *msg) {
    assert(vcpu->guest_state.exit.in_exit);
    vcpu->guest_state.exit.reason = msg[0];
    vcpu->guest_state.exit.qualification = msg[1];
    vcpu->guest_state.exit.instruction_length = msg[2];
    vcpu->guest_state.exit.guest_physical = msg[3];

    MACHINE_STATE_READ(vcpu->guest_state.machine.rflags, msg[4]);
    MACHINE_STATE_READ(vcpu->guest_state.machine.guest_interruptibility, msg[5]);
    MACHINE_STATE_READ(vcpu->guest_state.machine.control_entry, msg[6]);

    MACHINE_STATE_READ(vcpu->guest_state.machine.cr3, msg[7]);

    seL4_UserContext context;
    context.eip = msg[8];
    context.esp = msg[9];
    context.eflags = msg[10];
    context.eax = msg[11];
    context.ebx = msg[12];
    context.ecx = msg[13];
    context.edx = msg[14];
    context.esi = msg[15];
    context.edi = msg[16];
    context.ebp = msg[17];
    context.tls_base = msg[18];
    context.fs = msg[19];
    context.gs = msg[20];
    MACHINE_STATE_READ(vcpu->guest_state.machine.context, context);
}

void interrupt_pending_callback(vmm_t *vmm, seL4_Word badge)
{
    if (vmm->plat_callbacks.do_async(badge) == 0) {
        /* Got interrupt(s) from PIC, propagate to vcpus */

        /* TODO if all lapics are enabled, store which lapic
           (only one allowed) receives extints, and short circuit this */
        if (vmm->plat_callbacks.has_interrupt() != -1) {
            for (int i = 0; i < vmm->num_vcpus; i++) {
                vmm_vcpu_t *vcpu = &vmm->vcpus[i];

                if (!vmm_apic_enabled(vcpu->lapic)) {
                    /* LAPIC is disabled on this vcpu; bypass it */
                    vmm_vcpu_accept_interrupt(vcpu);
                } else if (vmm_apic_accept_pic_intr(vcpu)) {
                    /* LAPIC is enabled on this vcpu, and accepting
                       local extints from the pic */
                    assert(vmm_apic_local_deliver(vcpu, 0));
                } 
            }
        }
    }
}

/* Entry point of of VMM main host module. */
void vmm_run(vmm_t *vmm) {
    DPRINTF(2, "VMM MAIN HOST MODULE STARTED\n");

    for (int i = 0; i < vmm->num_vcpus; i++) {
        vmm_vcpu_t *vcpu = &vmm->vcpus[i];

        vcpu->guest_state.virt.interrupt_halt = 0;
        vcpu->guest_state.exit.in_exit = 0;

        /* sync the existing guest state */
        vmm_sync_guest_state(vcpu);
        vmm_sync_guest_context(vcpu);
        /* now invalidate everything */
        assert(vmm_guest_state_no_modified(&vcpu->guest_state));
        vmm_guest_state_invalidate_all(&vcpu->guest_state);
    }

    /* Start the boot vcpu guest thread running */
    seL4_TCB_Resume(vmm->vcpus[BOOT_VCPU].guest_tcb);

    /* Get our interrupt pending callback happening */
    seL4_TCB_BindAEP(simple_get_init_cap(&vmm->host_simple, seL4_CapInitThreadTCB), vmm->plat_callbacks.get_async_event_aep());

    while (1) {
        /* Block and wait for incoming msg or VM exits. */
        seL4_Word badge;
        seL4_MessageInfo_t msg = seL4_Wait(vmm->guest_fault_ep, &badge);

        seL4_Word msg_len = seL4_MessageInfo_get_length(msg);


        if (badge >= vmm->num_vcpus) {
            /* assume interrupt */
            interrupt_pending_callback(vmm, badge);
            continue;
        }

        /* We only accept guest faults */
        assert(msg_len == LIB_VMM_VM_EXIT_MSG_LEN);

        seL4_Word fault_message[LIB_VMM_VM_EXIT_MSG_LEN];
        for (int i = 0 ; i < LIB_VMM_VM_EXIT_MSG_LEN; i++) {
            fault_message[i] = seL4_GetMR(i);
        }

        vmm_vcpu_t *vcpu = &vmm->vcpus[badge];

        vka_cnode_saveCaller(&vcpu->reply_slot);

        /* Set all our state to invalid as we just received a fault and
         * none of it can conceivably be valid */
        vmm_guest_state_invalidate_all(&vcpu->guest_state);

        /* We in a fault */
        vcpu->guest_state.exit.in_exit = 1;
        /* Save the information we got in the fault */
        vmm_update_guest_state_from_fault(vcpu, fault_message);

        /* Handle the vm exit */
        vmm_handle_vm_exit(vcpu);

        DPRINTF(5, "VMM main host blocking for another message...\n");
    }

}

int vmm_exception_handler(vmm_vcpu_t *vcpu) 
{
    /* TODO this is here to ignore exceptions when linux tries to set
       segment selectors during smp bootup of ap vcpus. fix this properly! */
    if (vcpu->vcpu_id > 0) {
        printf("noticed exception on vcpu %d\n", vcpu->vcpu_id);
        //vmm_print_guest_context(0, vcpu);
        //vmm_set_user_context(&vcpu->guest_state, USER_CONTEXT_EAX, 0x10);
    }
    vmm_guest_exit_next_instruction(&vcpu->guest_state);
    return 0;
}

static void vmm_exit_init(vmm_t *vmm) {
    /* Connect VM exit handlers to correct function pointers */
    vmm->vmexit_handlers[EXIT_REASON_PENDING_INTERRUPT] = vmm_pending_interrupt_handler;
    vmm->vmexit_handlers[EXIT_REASON_EXCEPTION_NMI] = vmm_exception_handler;
    vmm->vmexit_handlers[EXIT_REASON_CPUID] = vmm_cpuid_handler;
    vmm->vmexit_handlers[EXIT_REASON_MSR_READ] = vmm_rdmsr_handler;
    vmm->vmexit_handlers[EXIT_REASON_MSR_WRITE] = vmm_wrmsr_handler;
    vmm->vmexit_handlers[EXIT_REASON_EPT_VIOLATION] = vmm_ept_violation_handler;
    vmm->vmexit_handlers[EXIT_REASON_CR_ACCESS] = vmm_cr_access_handler;
    vmm->vmexit_handlers[EXIT_REASON_IO_INSTRUCTION] = vmm_io_instruction_handler;
/*    vmm->vmexit_handlers[EXIT_REASON_RDTSC] = vmm_rdtsc_instruction_handler;*/
    vmm->vmexit_handlers[EXIT_REASON_HLT] = vmm_hlt_handler;
    vmm->vmexit_handlers[EXIT_REASON_VMX_TIMER] = vmm_vmx_timer_handler;
}

int vmm_finalize(vmm_t *vmm) {
    int err;
    vmm_exit_init(vmm);

    for (int i = 0; i < vmm->num_vcpus; i++) {
        vmm_vcpu_t *vcpu = &vmm->vcpus[i];

        vmm_init_guest_thread_state(vcpu);
        err = vmm_io_port_init_guest(&vmm->io_port, &vmm->host_simple, vcpu->guest_vcpu);
        if (err) {
            return err;
        }
    }
    return 0;
}


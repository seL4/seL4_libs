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
#include <platsupport/arch/tsc.h>

#include "vmm/debug.h"
#include "vmm/vmm.h"
#include "vmm/interrupt.h"
#include "vmm/platform/boot_guest.h"

void vmm_sync_guest_context(vmm_vcpu_t *vcpu) {
    if (IS_MACHINE_STATE_MODIFIED(vcpu->guest_state.machine.context)) {
        seL4_VCPUContext context;
        context.eax = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EAX);
        context.ebx = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EBX);
        context.ecx = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_ECX);
        context.edx = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EDX);
        context.esi = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_ESI);
        context.edi = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EDI);
        context.ebp = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EBP);
        seL4_IA32_VCPU_WriteRegisters(vcpu->guest_vcpu, &context);
        /* Sync our context */
        MACHINE_STATE_SYNC(vcpu->guest_state.machine.context);
    }
}

void vmm_reply_vm_exit(vmm_vcpu_t *vcpu) {
    assert(vcpu->guest_state.exit.in_exit);

    if (IS_MACHINE_STATE_MODIFIED(vcpu->guest_state.machine.context)) {
        vmm_sync_guest_context(vcpu);
    }

    /* Before we resume the guest, ensure there is no dirty state around */
    assert(vmm_guest_state_no_modified(&vcpu->guest_state));
    vmm_guest_state_invalidate_all(&vcpu->guest_state);

    vcpu->guest_state.exit.in_exit = 0;
}

void vmm_sync_guest_state(vmm_vcpu_t *vcpu) {
    vmm_guest_state_sync_cr0(&vcpu->guest_state, vcpu->guest_vcpu);
    vmm_guest_state_sync_cr3(&vcpu->guest_state, vcpu->guest_vcpu);
    vmm_guest_state_sync_cr4(&vcpu->guest_state, vcpu->guest_vcpu);
    vmm_guest_state_sync_idt_base(&vcpu->guest_state, vcpu->guest_vcpu);
    vmm_guest_state_sync_idt_limit(&vcpu->guest_state, vcpu->guest_vcpu);
    vmm_guest_state_sync_gdt_base(&vcpu->guest_state, vcpu->guest_vcpu);
    vmm_guest_state_sync_gdt_limit(&vcpu->guest_state, vcpu->guest_vcpu);
    vmm_guest_state_sync_cs_selector(&vcpu->guest_state, vcpu->guest_vcpu);
}


/* Handle VM exit in VMM module. */
static void vmm_handle_vm_exit(vmm_vcpu_t *vcpu) {
    int reason = vmm_guest_exit_get_reason(&vcpu->guest_state);

    /* Distribute the task according to the exit info. */
    vmm_print_guest_context(3, vcpu);

    if (!vcpu->vmm->vmexit_handlers[reason]) {
        printf("VM_FATAL_ERROR ::: vm exit handler is NULL for reason 0x%x.\n", reason);
        vmm_print_guest_context(0, vcpu);
        vcpu->online = 0;
        return;
    }

    /* Call the handler. */
    if (vcpu->vmm->vmexit_handlers[reason](vcpu)) {
        printf("VM_FATAL_ERROR ::: vmexit handler return error\n");
        vmm_print_guest_context(0, vcpu);
        vcpu->online = 0;
        return;
    }

    /* Reply to the VM exit exception to resume guest. */
    vmm_sync_guest_state(vcpu);
    if (vcpu->guest_state.exit.in_exit && !vcpu->guest_state.virt.interrupt_halt) {
        /* Guest is blocked, but we are no longer halted. Reply to it */
        vmm_reply_vm_exit(vcpu);
    }
}

static void vmm_update_guest_state_from_interrupt(vmm_vcpu_t *vcpu, seL4_Word *msg) {
    vcpu->guest_state.machine.eip = msg[0];
    vcpu->guest_state.machine.control_ppc = msg[1];
    vcpu->guest_state.machine.control_entry = msg[2];
}

static void vmm_update_guest_state_from_fault(vmm_vcpu_t *vcpu, seL4_Word *msg) {
    assert(vcpu->guest_state.exit.in_exit);

    /* The interrupt state is a subset of the fault state */
    vmm_update_guest_state_from_interrupt(vcpu, msg);

    vcpu->guest_state.exit.reason = msg[3];
    vcpu->guest_state.exit.qualification = msg[4];
    vcpu->guest_state.exit.instruction_length = msg[5];
    vcpu->guest_state.exit.guest_physical = msg[6];

    MACHINE_STATE_READ(vcpu->guest_state.machine.rflags, msg[7]);
    MACHINE_STATE_READ(vcpu->guest_state.machine.guest_interruptibility, msg[8]);

    MACHINE_STATE_READ(vcpu->guest_state.machine.cr3, msg[9]);

    seL4_VCPUContext context;
    context.eax = msg[10];
    context.ebx = msg[11];
    context.ecx = msg[12];
    context.edx = msg[13];
    context.esi = msg[14];
    context.edi = msg[15];
    context.ebp = msg[16];
    MACHINE_STATE_READ(vcpu->guest_state.machine.context, context);
}

/* Entry point of of VMM main host module. */
void vmm_run(vmm_t *vmm) {
    int error;
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
    vmm->vcpus[BOOT_VCPU].online = 1;

    /* Get our interrupt pending callback happening */
    seL4_CPtr aep = vmm->plat_callbacks.get_async_event_aep();
    error = seL4_TCB_BindAEP(simple_get_init_cap(&vmm->host_simple, seL4_CapInitThreadTCB), vmm->plat_callbacks.get_async_event_aep());
    assert(error == seL4_NoError);

    while (1) {
        /* Block and wait for incoming msg or VM exits. */
        seL4_Word badge;
        int fault;

        vmm_vcpu_t *vcpu = &vmm->vcpus[BOOT_VCPU];

        if (vcpu->online && !vcpu->guest_state.virt.interrupt_halt && !vcpu->guest_state.exit.in_exit) {
            seL4_SetMR(0, vmm_guest_state_get_eip(&vcpu->guest_state));
            seL4_SetMR(1, vmm_guest_state_get_control_ppc(&vcpu->guest_state));
            seL4_SetMR(2, vmm_guest_state_get_control_entry(&vcpu->guest_state));
            fault = seL4_VMEnter(vcpu->guest_vcpu, &badge);

            if (fault) {
                /* We in a fault */
                vcpu->guest_state.exit.in_exit = 1;

                /* Update the guest state from a fault */
                seL4_Word fault_message[LIB_VMM_VM_FAULT_EXIT_MSG_LEN];
                for (int i = 0 ; i < LIB_VMM_VM_FAULT_EXIT_MSG_LEN; i++) {
                    fault_message[i] = seL4_GetMR(i);
                }
                vmm_guest_state_invalidate_all(&vcpu->guest_state);
                vmm_update_guest_state_from_fault(vcpu, fault_message);
            } else {
                /* update the guest state from a non fault */
                seL4_Word int_message[LIB_VMM_VM_INT_EXIT_MSG_LEN];
                for (int i = 0 ; i < LIB_VMM_VM_INT_EXIT_MSG_LEN; i++) {
                    int_message[i] = seL4_GetMR(i);
                }
                vmm_guest_state_invalidate_all(&vcpu->guest_state);
                vmm_update_guest_state_from_interrupt(vcpu, int_message);
            }
        } else {
            seL4_Wait(aep, &badge);
            fault = 0;
        }

        if (!fault) {
            assert(badge >= vmm->num_vcpus);
            /* assume interrupt */
            int raise = vmm->plat_callbacks.do_async(badge);
            if (raise == 0) {
                /* Check if this caused PIC to generate interrupt */
                vmm_check_external_interrupt(vmm);
            }

            continue;
        }

        /* Handle the vm exit */
        vmm_handle_vm_exit(vcpu);

        DPRINTF(5, "VMM main host blocking for another message...\n");
    }

}

static void vmm_exit_init(vmm_t *vmm) {
    /* Connect VM exit handlers to correct function pointers */
    vmm->vmexit_handlers[EXIT_REASON_PENDING_INTERRUPT] = vmm_pending_interrupt_handler;
    //vmm->vmexit_handlers[EXIT_REASON_EXCEPTION_NMI] = vmm_exception_handler;
    vmm->vmexit_handlers[EXIT_REASON_CPUID] = vmm_cpuid_handler;
    vmm->vmexit_handlers[EXIT_REASON_MSR_READ] = vmm_rdmsr_handler;
    vmm->vmexit_handlers[EXIT_REASON_MSR_WRITE] = vmm_wrmsr_handler;
    vmm->vmexit_handlers[EXIT_REASON_EPT_VIOLATION] = vmm_ept_violation_handler;
    vmm->vmexit_handlers[EXIT_REASON_CR_ACCESS] = vmm_cr_access_handler;
    vmm->vmexit_handlers[EXIT_REASON_IO_INSTRUCTION] = vmm_io_instruction_handler;
/*    vmm->vmexit_handlers[EXIT_REASON_RDTSC] = vmm_rdtsc_instruction_handler;*/
    vmm->vmexit_handlers[EXIT_REASON_HLT] = vmm_hlt_handler;
    vmm->vmexit_handlers[EXIT_REASON_VMX_TIMER] = vmm_vmx_timer_handler;
    vmm->vmexit_handlers[EXIT_REASON_VMCALL] = vmm_vmcall_handler;
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


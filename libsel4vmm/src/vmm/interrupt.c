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

/* vm exits and general handling of interrupt injection */

#include <stdio.h>
#include <stdlib.h>

#include <sel4/sel4.h>

#include "vmm/debug.h"
#include "vmm/vmm.h"
#include "vmm/processor/decode.h"

#define TRAMPOLINE_LENGTH (100)

static void resume_guest(vmm_vcpu_t *vcpu) {
    /* Disable exit-for-interrupt in guest state to allow the guest to resume. */
    uint32_t state = vmm_guest_state_get_control_ppc(&vcpu->guest_state);
    state &= ~BIT(2); /* clear the exit for interrupt flag */
    vmm_guest_state_set_control_ppc(&vcpu->guest_state, state);
}

static void inject_irq(vmm_vcpu_t *vcpu, int irq) {
    /* Inject a vectored exception into the guest */
    assert(irq >= 16);
    vmm_guest_state_set_control_entry(&vcpu->guest_state, BIT(31) | irq);
}

void vmm_inject_exception(vmm_vcpu_t *vcpu, int exception, int has_error, uint32_t error_code) {
    assert(exception < 16);
    // ensure we are not already injecting an interrupt or exception
    uint32_t int_control = vmm_guest_state_get_control_entry(&vcpu->guest_state);
    if ( (int_control & BIT(31)) != 0) {
        ZF_LOGF("Cannot inject exception");
    }
    if (has_error) {
        vmm_guest_state_set_entry_exception_error_code(&vcpu->guest_state, error_code);
    }
    vmm_guest_state_set_control_entry(&vcpu->guest_state, BIT(31) | exception | 3 << 8 | (has_error ? BIT(11) : 0));
}

void wait_for_guest_ready(vmm_vcpu_t *vcpu) {
    /* Request that the guest exit at the earliest point that we can inject an interrupt. */
    uint32_t state = vmm_guest_state_get_control_ppc(&vcpu->guest_state);
    state |= BIT(2); /* set the exit for interrupt flag */
    vmm_guest_state_set_control_ppc(&vcpu->guest_state, state);
}

int can_inject(vmm_vcpu_t *vcpu) {
    uint32_t rflags = vmm_guest_state_get_rflags(&vcpu->guest_state, vcpu->guest_vcpu);
    uint32_t guest_int = vmm_guest_state_get_interruptibility(&vcpu->guest_state, vcpu->guest_vcpu);
    uint32_t int_control = vmm_guest_state_get_control_entry(&vcpu->guest_state);

    /* we can only inject if the interrupt mask flag is not set in flags,
       guest is not in an uninterruptable state and we are not already trying to
       inject an interrupt */

    if ( (rflags & BIT(9)) && (guest_int & 0xF) == 0 && (int_control & BIT(31)) == 0) {
        return 1;
    }
    return 0;
}

/* This function is called by the local apic when a new interrupt has occured. */
void vmm_have_pending_interrupt(vmm_vcpu_t *vcpu) {
    if (vmm_apic_has_interrupt(vcpu) >= 0) {
        /* there is actually an interrupt to inject */
        if (can_inject(vcpu)) {
            if (vcpu->guest_state.virt.interrupt_halt) {
                /* currently halted. need to put the guest
                 * in a state where it can inject again */
                wait_for_guest_ready(vcpu);
                vcpu->guest_state.virt.interrupt_halt = 0;
                vmm_sync_guest_state(vcpu);
                vmm_reply_vm_exit(vcpu); /* unblock the guest */
            } else {
                int irq = vmm_apic_get_interrupt(vcpu);
                inject_irq(vcpu, irq);
                /* see if there are more */
                if (vmm_apic_has_interrupt(vcpu) >= 0) {
                    wait_for_guest_ready(vcpu);
                }
            }
        } else {
            wait_for_guest_ready(vcpu);
            if (vcpu->guest_state.virt.interrupt_halt) {
                vcpu->guest_state.virt.interrupt_halt = 0;
                vmm_sync_guest_state(vcpu);
                vmm_reply_vm_exit(vcpu); /* unblock the guest */
            }
        }
    }
}

int vmm_pending_interrupt_handler(vmm_vcpu_t *vcpu) {
    /* see if there is actually a pending interrupt */
    assert(can_inject(vcpu));
    int irq = vmm_apic_get_interrupt(vcpu);
    if (irq == -1) {
        resume_guest(vcpu);
    } else {
        /* inject the interrupt */
        inject_irq(vcpu, irq);
        if (!(vmm_apic_has_interrupt(vcpu) >= 0)) {
            resume_guest(vcpu);
        }
        vcpu->guest_state.virt.interrupt_halt = 0;
    }
    return 0;
}

/* Start an AP vcpu after a sipi with the requested vector */
void vmm_start_ap_vcpu(vmm_vcpu_t *vcpu, unsigned int sipi_vector)
{
    DPRINTF(1, "trying to start vcpu %d\n", vcpu->vcpu_id);

    uint16_t segment = sipi_vector * 0x100;
    uintptr_t eip = sipi_vector * 0x1000;
    guest_state_t *gs = &vcpu->guest_state;

    /* Emulate up to 100 bytes of trampoline code */
    uint8_t instr[TRAMPOLINE_LENGTH];
    vmm_fetch_instruction(vcpu, eip, vmm_guest_state_get_cr3(gs, vcpu->guest_vcpu),
            TRAMPOLINE_LENGTH, instr);

    eip = vmm_emulate_realmode(&vcpu->vmm->guest_mem, instr, &segment, eip,
            TRAMPOLINE_LENGTH, gs);

    vmm_guest_state_set_eip(&vcpu->guest_state, eip);

    vmm_sync_guest_context(vcpu);
    vmm_sync_guest_state(vcpu);

    assert(!"no tcb");
//    seL4_TCB_Resume(vcpu->guest_tcb);
}

/* Got interrupt(s) from PIC, propagate to relevant vcpu lapic */
void vmm_check_external_interrupt(vmm_t *vmm)
{
    /* TODO if all lapics are enabled, store which lapic
       (only one allowed) receives extints, and short circuit this */
    if (vmm->plat_callbacks.has_interrupt() != -1) {
        for (int i = 0; i < vmm->num_vcpus; i++) {
            vmm_vcpu_t *vcpu = &vmm->vcpus[i];
            if (vmm_apic_accept_pic_intr(vcpu)) {
                vmm_vcpu_accept_interrupt(vcpu);
                break; /* Only one VCPU can take a PIC interrupt */
            }
        }
    }
}

void vmm_vcpu_accept_interrupt(vmm_vcpu_t *vcpu)
{
    if (vmm_apic_has_interrupt(vcpu) == -1) {
        return;
    }

    /* in an exit, can call the regular injection method */
    vmm_have_pending_interrupt(vcpu);
}

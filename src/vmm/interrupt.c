/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/* vm exits and general handling of interrupt injection */

#include <stdio.h>
#include <stdlib.h>

#include <sel4/sel4.h>

#include "vmm/vmm.h"

static void resume_guest(vmm_vcpu_t *vcpu) {
    /* Disable exit-for-interrupt in guest state to allow the guest to resume. */
    uint32_t state = vmm_guest_state_get_control_ppc(&vcpu->guest_state, vcpu->guest_vcpu);
    state &= ~BIT(2); /* clear the exit for interrupt flag */
    vmm_guest_state_set_control_ppc(&vcpu->guest_state, state);
}

static void inject_irq(vmm_vcpu_t *vcpu, int irq) {
    /* Inject an IRQ into the guest's execution state. */
    assert((irq & 0xff) == irq);
    irq &= 0xff;
    vmm_guest_state_set_control_entry(&vcpu->guest_state, BIT(31) | irq);
}

void wait_for_guest_ready(vmm_vcpu_t *vcpu) {
    /* Request that the guest exit at the earliest point that we can inject an interrupt. */
    uint32_t state = vmm_guest_state_get_control_ppc(&vcpu->guest_state, vcpu->guest_vcpu);
    state |= BIT(2); /* set the exit for interrupt flag */
    vmm_guest_state_set_control_ppc(&vcpu->guest_state, state);
}

int can_inject(vmm_vcpu_t *vcpu) {
    uint32_t rflags = vmm_guest_state_get_rflags(&vcpu->guest_state, vcpu->guest_vcpu);
    uint32_t guest_int = vmm_guest_state_get_interruptibility(&vcpu->guest_state, vcpu->guest_vcpu);
    uint32_t int_control = vmm_guest_state_get_control_entry(&vcpu->guest_state, vcpu->guest_vcpu);

    /* we can only inject if the interrupt mask flag is not set in flags,
       guest is not in an uninterruptable state and we are not already trying to
       inject an interrupt */

    if ( (rflags & BIT(9)) && (guest_int & 0xF) == 0 && (int_control & BIT(31)) == 0) {
        return 1;
    }
    return 0;
}

/* XXX These should be cleaned up when 8259 is moved into here */
int get_interrupt(vmm_vcpu_t *vcpu)
{
	if (vmm_apic_enabled(vcpu->lapic)) {
		return vmm_apic_get_interrupt(vcpu);
	} else {
		// Use PIC if LAPIC is disabled
		return vcpu->vmm->plat_callbacks.has_interrupt();
	}
}

int has_interrupt(vmm_vcpu_t *vcpu)
{
	if (vmm_apic_enabled(vcpu->lapic)) {
		return vmm_apic_has_interrupt(vcpu);
	} else {
		// Use PIC if LAPIC is disabled
		return vcpu->vmm->plat_callbacks.has_interrupt();
	}
}

void vmm_have_pending_interrupt(vmm_vcpu_t *vcpu) {
    /* This function is called when a new interrupt has occured. */
    if (can_inject(vcpu)) {
        int irq = get_interrupt(vcpu);
        if (irq != -1) {
            /* there is actually an interrupt to inject */
            if (vcpu->guest_state.virt.interrupt_halt) {
                /* currently halted. need to put the guest
                 * in a state where it can inject again */
                wait_for_guest_ready(vcpu);
                vcpu->guest_state.virt.interrupt_halt = 0;
            } else {
                inject_irq(vcpu, irq);
                /* see if there are more */
                if (has_interrupt(vcpu)) {
                    wait_for_guest_ready(vcpu);
                }
            }
        }
    } else {
        wait_for_guest_ready(vcpu);
        vcpu->guest_state.virt.interrupt_halt = 0;
    }
}

int vmm_pending_interrupt_handler(vmm_vcpu_t *vcpu) {
    /* see if there is actually a pending interrupt */
    assert(can_inject(vcpu));
    int irq = vcpu->vmm->plat_callbacks.get_interrupt();
    if (irq == -1) {
        resume_guest(vcpu);
    } else {
        /* inject the interrupt */
        inject_irq(vcpu, irq);
        if (!vcpu->vmm->plat_callbacks.has_interrupt()) {
            resume_guest(vcpu);
        }
        vcpu->guest_state.virt.interrupt_halt = 0;
    }
    return 0;
}

/* Start an AP vcpu after a sipi with the requested vector */
void vmm_start_ap_vcpu(vmm_vcpu_t *vcpu, unsigned int sipi_vector)
{
	/* See /arch/x86/kvm/x86.c:7027 Linux 3.18 */
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_CS_SELECTOR, sipi_vector << 8);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_CS_BASE, sipi_vector << 12);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_SYSENTER_EIP, 0);
	seL4_TCB_Resume(vcpu->guest_tcb);
}

void vmm_vcpu_accept_interrupt(vmm_vcpu_t *vcpu)
{
    /* TODO: there is a really annoying race between our need to stop the guest thread
     * so that we can inspect its state and potentially inject an interrupt (this
     * is done in vmm_have_pending_interrupt). Unfortunatley if we just stop it
     * and it has decided to fault, we will lose the fault message. One option
     * is a 'SuspendIf' style syscall, however this requires kernel changes
     * that are maybe disagreeable. The other option is to ask the kernel to atomically
     * inspect the state, inject if possible, and the ntell you waht happened. THis
     * is the approach we will eventually use, but it requires 'locking' the interrupt
     * controller such that you can poll the current interrupt, and then still have
     * that be the current pending interrupt after calling the kernel and then
     * knowing if it was actually injected or not. Do this once interrupt overhead
     * starts to matter */
	if (vcpu->guest_state.exit.in_exit) {
		/* in an exit, can call the regular injection method */
		vmm_have_pending_interrupt(vcpu);
		vmm_sync_guest_state(vcpu);
	} else {
		/* Have some interrupts to inject, but we need to do this with
		 * the guest not running */
		wait_for_guest_ready(vcpu);
		vmm_guest_state_sync_control_ppc(&vcpu->guest_state, vcpu->guest_vcpu);
	}
#if 0
    int did_suspend = 0;
    /* Pause the guest so we can sensibly inspect its state, unless it is
     * already blocked */
    if (!vcpu->guest_state.exit.in_exit) {
        int state = seL4_TCB_SuspendIf(vmm->guest_tcb);
        if (!state) {
            did_suspend = 1;
            /* All state is invalid as the guest was running */
            vmm_guest_state_invalidate_all(&vcpu->guest_state);
        }
    }
    vmm_have_pending_interrupt(vmm);
    vmm_sync_guest_state(vmm);
    if (did_suspend) {
        seL4_TCB_Resume(vmm->guest_tcb);
        /* Guest is running, everything invalid again */
        vmm_guest_state_invalidate_all(&vcpu->guest_state);
    }
#endif
}


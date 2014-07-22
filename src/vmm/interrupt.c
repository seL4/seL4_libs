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

static void resume_guest(vmm_t *vmm) {
    /* Disable exit-for-interrupt in guest state to allow the guest to resume. */
    uint32_t state = vmm_guest_state_get_control_ppc(&vmm->guest_state, vmm->guest_vcpu);
    state &= ~BIT(2); /* clear the exit for interrupt flag */
    vmm_guest_state_set_control_ppc(&vmm->guest_state, state);
}

static void inject_irq(vmm_t *vmm, int irq) {
    /* Inject an IRQ into the guest's execution state. */
    assert((irq & 0xff) == irq);
    irq &= 0xff;
    vmm_guest_state_set_control_entry(&vmm->guest_state, BIT(31) | irq);
}

void wait_for_guest_ready(vmm_t *vmm) {
    /* Request that the guest exit at the earliest point that we can inject an interrupt. */
    uint32_t state = vmm_guest_state_get_control_ppc(&vmm->guest_state, vmm->guest_vcpu);
    state |= BIT(2); /* set the exit for interrupt flag */
    vmm_guest_state_set_control_ppc(&vmm->guest_state, state);
}

int can_inject(vmm_t *vmm) {
    uint32_t rflags = vmm_guest_state_get_rflags(&vmm->guest_state, vmm->guest_vcpu);
    uint32_t guest_int = vmm_guest_state_get_interruptibility(&vmm->guest_state, vmm->guest_vcpu);
    uint32_t int_control = vmm_guest_state_get_control_entry(&vmm->guest_state, vmm->guest_vcpu);

    /* we can only inject if the interrupt mask flag is not set in flags,
       guest is not in an uninterruptable state and we are not already trying to
       inject an interrupt */

    if ( (rflags & BIT(9)) && (guest_int & 0xF) == 0 && (int_control & BIT(31)) == 0) {
        return 1;
    }
    return 0;
}

void vmm_have_pending_interrupt(vmm_t *vmm) {
    /* This function is called when a new interrupt has occured. */
    if (can_inject(vmm)) {
        int irq = vmm->plat_callbacks.get_interrupt();
        if (irq != -1) {
            /* there is actually an interrupt to inject */
            if (vmm->guest_state.virt.interrupt_halt) {
                /* currently halted. need to put the guest
                 * in a state where it can inject again */
                wait_for_guest_ready(vmm);
                vmm->guest_state.virt.interrupt_halt = 0;
            } else {
                inject_irq(vmm, irq);
                /* see if there are more */
                if (vmm->plat_callbacks.has_interrupt()) {
                    wait_for_guest_ready(vmm);
                }
            }
        }
    } else {
        wait_for_guest_ready(vmm);
        vmm->guest_state.virt.interrupt_halt = 0;
    }
}

int vmm_pending_interrupt_handler(vmm_t *vmm) {
    /* see if there is actually a pending interrupt */
    assert(can_inject(vmm));
    int irq = vmm->plat_callbacks.get_interrupt();
    if (irq == -1) {
        resume_guest(vmm);
    } else {
        /* inject the interrupt */
        inject_irq(vmm, irq);
        if (!vmm->plat_callbacks.has_interrupt()) {
            resume_guest(vmm);
        }
        vmm->guest_state.virt.interrupt_halt = 0;
    }
    return 0;
}

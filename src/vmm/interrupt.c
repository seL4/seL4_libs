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
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include <sel4/sel4.h>

#include "vmm/config.h"
#include "vmm/vmm.h"
#include "vmm/vmexit.h"
#include "vmm/vmcs.h"
#include "vmm/helper.h"
#include "vmm/driver/int_manger.h"

#define DEBUG_INTERRUPT_LVL 5

static void resume_guest(gcb_t *guest) {
    /* Disable exit-for-interrupt in guest state to allow the guest to resume. */
    dprintf(DEBUG_INTERRUPT_LVL, "vmm int : guest%d allowed to resume.\n",
            VMM_GUEST_BADGE_TO_ID(guest->sender));
    uint32_t state = vmm_vmcs_read(LIB_VMM_VCPU_CAP, VMX_CONTROL_PRIMARY_PROCESSOR_CONTROLS);
    state &= ~BIT(2); /* clear the exit for interrupt flag */
    vmm_vmcs_write(LIB_VMM_VCPU_CAP, VMX_CONTROL_PRIMARY_PROCESSOR_CONTROLS, state);
}

static void inject_irq(int irq, gcb_t *guest) {
    /* Inject an IRQ into the guest's execution state. */
    assert((irq & 0xff) == irq);
    irq &= 0xff;
    vmm_vmcs_write(LIB_VMM_VCPU_CAP, VMX_CONTROL_ENTRY_INTERRUPTION_INFO,  BIT(31) | irq);
}

static void wait_for_guest_ready(gcb_t *guest) {
    /* Request that the guest exit at the earliest point that we can inject an interrupt. */
    dprintf(DEBUG_INTERRUPT_LVL, "vmm int : guest%d forced to exit for interrupt.\n",
            VMM_GUEST_BADGE_TO_ID(guest->sender));
    uint32_t state = vmm_vmcs_read(LIB_VMM_VCPU_CAP, VMX_CONTROL_PRIMARY_PROCESSOR_CONTROLS);
    state |= BIT(2); /* set the exit for interrupt flag */
    vmm_vmcs_write(LIB_VMM_VCPU_CAP, VMX_CONTROL_PRIMARY_PROCESSOR_CONTROLS, state);
}

static int can_inject(gcb_t *guest) {
    uint32_t rflags = vmm_vmcs_read(LIB_VMM_VCPU_CAP, VMX_GUEST_RFLAGS);
    uint32_t guest_int = vmm_vmcs_read(LIB_VMM_VCPU_CAP, VMX_GUEST_INTERRUPTABILITY);
    uint32_t int_control = vmm_vmcs_read(LIB_VMM_VCPU_CAP, VMX_CONTROL_ENTRY_INTERRUPTION_INFO);
    
    /* we can only inject if the interrupt mask flag is not set in flags,
       guest is not in an uninterruptable state and we are not already trying to
       inject an interrupt */

    if ( (rflags & BIT(9)) && (guest_int & 0xF) == 0 && (int_control & BIT(31)) == 0) {
        return 1;
    }
    return 0;
}

static int message_driver(gcb_t *guest, int message) {
    seL4_MessageInfo_t msgInfo = seL4_MessageInfo_new(message, 0, 0, 1);

    seL4_SetMR(0, guest->sender);

    msgInfo = seL4_Call(LIB_VMM_DRIVER_INTERRUPT_CAP, msgInfo);
    assert(seL4_MessageInfo_get_length(msgInfo) == 1);
    return seL4_GetMR(0);
}

static int get_interrupt(gcb_t *guest) {
    return message_driver(guest, LABEL_INT_MAN_GET_INTERRUPT);
}

int interrupt_pending(gcb_t *guest) {
    return message_driver(guest, LABEL_INT_MAN_HAS_INTERRUPT);
}

void vmm_have_pending_interrupt(gcb_t *guest) {
    /* This function is called when a new interrupt has occured. */
    if (can_inject(guest)) {
        int irq = get_interrupt(guest);
        if (irq != -1) {
            /* there is actually an interrupt to inject */
            if (guest->interrupt_halt) {
                /* currently halted. need to put the guest
                 * in a state where it can inject again */
                wait_for_guest_ready(guest);
                guest->interrupt_halt = 0;
            } else {
                inject_irq(irq, guest);
                /* see if there are more */
                if (interrupt_pending(guest)) {
                    wait_for_guest_ready(guest);
                }
            }
        }
    } else {
        wait_for_guest_ready(guest);
        guest->interrupt_halt = 0;
    }
}

int vmm_pending_interrupt_handler(gcb_t *guest) {
    assert(guest->sender >= LIB_VMM_GUEST_OS_BADGE_START);
    dprintf(DEBUG_INTERRUPT_LVL, "vmm int : guest%d exited for pending interrupt.\n",
            VMM_GUEST_BADGE_TO_ID(guest->sender));
    /* see if there is actually a pending interrupt */
    int irq = get_interrupt(guest);
    if (irq == -1) {
        resume_guest(guest);
    } else {
        /* inject the interrupt */
        inject_irq(irq, guest);
        if (!interrupt_pending(guest)) {
            resume_guest(guest);
        }
    }
    guest->interrupt_halt = 0;
    return LIB_VMM_SUCC;
}

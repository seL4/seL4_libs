/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/*vm exits related with vmx timer*/

#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>

#include <sel4/sel4.h>

#include "vmm/vmm.h"
#include "vmm/platform/vmcs.h"

int vmm_vmx_timer_handler(vmm_t *vmm) {
#ifdef CONFIG_LIB_VMM_VMX_TIMER_DEBUG
    vmm_print_guest_context(0, vmm);
//    vmm_vmcs_write(vmm->guest_vcpu, VMX_CONTROL_PIN_EXECUTION_CONTROLS, vmm_vmcs_read(vmm->guest_vcpu, VMX_CONTROL_PIN_EXECUTION_CONTROLS) | BIT(6));
    vmm_vmcs_write(vmm->guest_vcpu, VMX_GUEST_VMX_PREEMPTION_TIMER_VALUE, CONFIG_LIB_VMM_VMX_TIMER_TIMEOUT);
    return 0;
#else
    return -1;
#endif
}


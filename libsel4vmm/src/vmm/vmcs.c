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

#include <stdio.h>
#include <stdlib.h>

#include <sel4/sel4.h>

#include "vmm/vmm.h"
#include "vmm/platform/vmcs.h"

int vmm_vmcs_read(seL4_CPtr vcpu, seL4_Word field) {

    seL4_X86_VCPU_ReadVMCS_t UNUSED result;

    assert(vcpu);

    result = seL4_X86_VCPU_ReadVMCS(vcpu, field);
    assert(result.error == seL4_NoError);
    return result.value;
}

/*write a field and its value into the VMCS*/
void vmm_vmcs_write(seL4_CPtr vcpu, seL4_Word field, seL4_Word value) {

    seL4_X86_VCPU_WriteVMCS_t UNUSED result;
    assert(vcpu);

    result = seL4_X86_VCPU_WriteVMCS(vcpu, field, value);
    assert(result.error == seL4_NoError);
}

/*init the vmcs structure for a guest os thread*/
void vmm_vmcs_init_guest(vmm_vcpu_t *vcpu) {
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_ES_SELECTOR, 2 << 3);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_CS_SELECTOR, BIT(3));
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_SS_SELECTOR, 2 << 3);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_DS_SELECTOR, 2 << 3);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_FS_SELECTOR, 0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_GS_SELECTOR, 0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_LDTR_SELECTOR, 0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_TR_SELECTOR, 0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_ES_LIMIT, ~0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_CS_LIMIT, ~0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_SS_LIMIT, ~0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_DS_LIMIT, ~0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_FS_LIMIT, 0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_GS_LIMIT, 0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_LDTR_LIMIT, 0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_TR_LIMIT, 0x0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_GDTR_LIMIT, 0x0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_IDTR_LIMIT, 0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_ES_ACCESS_RIGHTS, 0xC093);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_CS_ACCESS_RIGHTS, 0xC09B);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_SS_ACCESS_RIGHTS, 0xC093);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_DS_ACCESS_RIGHTS, 0xC093);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_FS_ACCESS_RIGHTS, BIT(16));
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_GS_ACCESS_RIGHTS, BIT(16));
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_LDTR_ACCESS_RIGHTS, BIT(16));
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_TR_ACCESS_RIGHTS, 0x8B);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_SYSENTER_CS, 0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_CONTROL_CR0_MASK, vcpu->guest_state.virt.cr.cr0_mask);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_CONTROL_CR4_MASK, vcpu->guest_state.virt.cr.cr4_mask);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_CONTROL_CR0_READ_SHADOW, vcpu->guest_state.virt.cr.cr0_shadow);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_CONTROL_CR4_READ_SHADOW, vcpu->guest_state.virt.cr.cr4_shadow);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_ES_BASE, 0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_CS_BASE, 0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_SS_BASE, 0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_DS_BASE, 0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_FS_BASE, 0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_GS_BASE, 0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_LDTR_BASE, 0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_TR_BASE, 0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_GDTR_BASE, 0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_IDTR_BASE, 0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_RFLAGS, BIT(1));
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_SYSENTER_ESP, 0);
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_SYSENTER_EIP, 0);
    vcpu->guest_state.machine.control_ppc = VMX_CONTROL_PPC_HLT_EXITING | VMX_CONTROL_PPC_CR3_LOAD_EXITING | VMX_CONTROL_PPC_CR3_STORE_EXITING;
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_CONTROL_PRIMARY_PROCESSOR_CONTROLS, vcpu->guest_state.machine.control_ppc);
    vcpu->guest_state.machine.control_entry = vmm_vmcs_read(vcpu->guest_vcpu, VMX_CONTROL_ENTRY_INTERRUPTION_INFO);

#ifdef CONFIG_LIB_VMM_VMX_TIMER_DEBUG
    /* Enable pre-emption timer */
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_CONTROL_PIN_EXECUTION_CONTROLS, BIT(6));
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_CONTROL_EXIT_CONTROLS, BIT(22));
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_GUEST_VMX_PREEMPTION_TIMER_VALUE, CONFIG_LIB_VMM_VMX_TIMER_TIMEOUT);
#endif
}

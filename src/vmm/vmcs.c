/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include <stdio.h>
#include <stdint.h>
#include <sel4/sel4.h>
#include <assert.h>
#include "vmm/vmcs.h"
#include "vmm/vmm.h"



int vmm_vmcs_read(seL4_CPtr vcpu, unsigned int field) {

    seL4_IA32_VCPU_ReadVMCS_t result;
    
    assert(vcpu);

    result = seL4_IA32_VCPU_ReadVMCS(vcpu, field);
    assert(result.error == seL4_NoError);
    return result.value;
}


/*write a field and its value into the VMCS*/
void vmm_vmcs_write(seL4_CPtr vcpu, unsigned int field, unsigned int value) {
    
    seL4_IA32_VCPU_WriteVMCS_t result;
    assert(vcpu);

    result = seL4_IA32_VCPU_WriteVMCS(vcpu, field, value);
    assert(result.error == seL4_NoError);
}


/*init the vmcs structure for a guest os thread*/
void vmm_vmcs_init_guest(seL4_CPtr vcpu, seL4_Word pd) {
    
    assert(vcpu);

    vmm_vmcs_write(vcpu, VMX_GUEST_ES_SELECTOR, 2 << 3);
    vmm_vmcs_write(vcpu, VMX_GUEST_CS_SELECTOR, 1 << 3);
    vmm_vmcs_write(vcpu, VMX_GUEST_SS_SELECTOR, 2 << 3);
    vmm_vmcs_write(vcpu, VMX_GUEST_DS_SELECTOR, 2 << 3);
    vmm_vmcs_write(vcpu, VMX_GUEST_FS_SELECTOR, 0);
    vmm_vmcs_write(vcpu, VMX_GUEST_GS_SELECTOR, 0);
    vmm_vmcs_write(vcpu, VMX_GUEST_LDTR_SELECTOR, 0);
    vmm_vmcs_write(vcpu, VMX_GUEST_TR_SELECTOR, 0);
    vmm_vmcs_write(vcpu, VMX_GUEST_ES_LIMIT, ~0);
    vmm_vmcs_write(vcpu, VMX_GUEST_CS_LIMIT, ~0);
    vmm_vmcs_write(vcpu, VMX_GUEST_SS_LIMIT, ~0);
    vmm_vmcs_write(vcpu, VMX_GUEST_DS_LIMIT, ~0);
    vmm_vmcs_write(vcpu, VMX_GUEST_FS_LIMIT, 0);
    vmm_vmcs_write(vcpu, VMX_GUEST_GS_LIMIT, 0);
    vmm_vmcs_write(vcpu, VMX_GUEST_LDTR_LIMIT, 0);
    vmm_vmcs_write(vcpu, VMX_GUEST_TR_LIMIT, 0x0);
    vmm_vmcs_write(vcpu, VMX_GUEST_GDTR_LIMIT, 0x0);
    vmm_vmcs_write(vcpu, VMX_GUEST_IDTR_LIMIT, 0);
    vmm_vmcs_write(vcpu, VMX_GUEST_ES_ACCESS_RIGHTS, 0xC093);
    vmm_vmcs_write(vcpu, VMX_GUEST_CS_ACCESS_RIGHTS, 0xC09B);
    vmm_vmcs_write(vcpu, VMX_GUEST_SS_ACCESS_RIGHTS, 0xC093);
    vmm_vmcs_write(vcpu, VMX_GUEST_DS_ACCESS_RIGHTS, 0xC093);
    vmm_vmcs_write(vcpu, VMX_GUEST_FS_ACCESS_RIGHTS, BIT(16));
    vmm_vmcs_write(vcpu, VMX_GUEST_GS_ACCESS_RIGHTS, BIT(16));
    vmm_vmcs_write(vcpu, VMX_GUEST_LDTR_ACCESS_RIGHTS, BIT(16));
    vmm_vmcs_write(vcpu, VMX_GUEST_TR_ACCESS_RIGHTS, 0x8B);
    vmm_vmcs_write(vcpu, VMX_GUEST_SYSENTER_CS, 0);
    vmm_vmcs_write(vcpu, VMX_GUEST_CR0, VMM_VMCS_CR0_SHADOW);
    vmm_vmcs_write(vcpu, VMX_CONTROL_CR0_MASK, VMM_VMCS_CR0_MASK);
    vmm_vmcs_write(vcpu, VMX_CONTROL_CR4_MASK, VMM_VMCS_CR4_MASK);
    vmm_vmcs_write(vcpu, VMX_CONTROL_CR0_READ_SHADOW, VMM_VMCS_CR0_SHADOW);
    vmm_vmcs_write(vcpu, VMX_CONTROL_CR4_READ_SHADOW, VMM_VMCS_CR4_SHADOW);
    vmm_vmcs_write(vcpu, VMX_GUEST_CR3, pd);
    vmm_vmcs_write(vcpu, VMX_GUEST_CR4, VMM_VMCS_CR4_SHADOW); /* We want large pages for the initial pagetable. */
    vmm_vmcs_write(vcpu, VMX_GUEST_ES_BASE, 0);
    vmm_vmcs_write(vcpu, VMX_GUEST_CS_BASE, 0);
    vmm_vmcs_write(vcpu, VMX_GUEST_SS_BASE, 0);
    vmm_vmcs_write(vcpu, VMX_GUEST_DS_BASE, 0);
    vmm_vmcs_write(vcpu, VMX_GUEST_FS_BASE, 0);
    vmm_vmcs_write(vcpu, VMX_GUEST_GS_BASE, 0);
    vmm_vmcs_write(vcpu, VMX_GUEST_LDTR_BASE, 0);
    vmm_vmcs_write(vcpu, VMX_GUEST_TR_BASE, 0);
    vmm_vmcs_write(vcpu, VMX_GUEST_GDTR_BASE, 0);
    vmm_vmcs_write(vcpu, VMX_GUEST_IDTR_BASE, 0);
    vmm_vmcs_write(vcpu, VMX_GUEST_RFLAGS, BIT(1));
    vmm_vmcs_write(vcpu, VMX_GUEST_SYSENTER_ESP, 0);
    vmm_vmcs_write(vcpu, VMX_GUEST_SYSENTER_EIP, 0);
    vmm_vmcs_write(vcpu, VMX_CONTROL_PRIMARY_PROCESSOR_CONTROLS, BIT(7));
}



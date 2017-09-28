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

#pragma once

#include <vmm/interrupt.h>
#include <vmm/vmm.h>

typedef int(*vmexit_handler_ptr)(vmm_vcpu_t *vcpu);

/*vm exit handlers*/
int vmm_cpuid_handler(vmm_vcpu_t *vcpu);
int vmm_ept_violation_handler(vmm_vcpu_t *vcpu);
int vmm_wrmsr_handler(vmm_vcpu_t *vcpu);
int vmm_rdmsr_handler(vmm_vcpu_t *vcpu);
int vmm_io_instruction_handler(vmm_vcpu_t *vcpu);
int vmm_hlt_handler(vmm_vcpu_t *vcpu);
int vmm_vmx_timer_handler(vmm_vcpu_t *vcpu);
int vmm_cr_access_handler(vmm_vcpu_t *vcpu);
int vmm_vmcall_handler(vmm_vcpu_t *vcpu);


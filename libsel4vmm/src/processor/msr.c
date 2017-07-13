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

/*handling msr read & write exceptions*/

#include <stdio.h>
#include <stdlib.h>

#include <sel4/sel4.h>

#include "vmm/debug.h"
#include "vmm/vmm.h"
#include "vmm/processor/msr.h"

int vmm_rdmsr_handler(vmm_vcpu_t *vcpu) {

    int ret = 0;
    unsigned int msr_no = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_ECX);
    uint64_t data = 0;

    DPRINTF(4, "rdmsr ecx 0x%x\n", msr_no);

    // src reference: Linux kernel 3.11 kvm arch/x86/kvm/x86.c
    switch (msr_no) {
        case MSR_IA32_PLATFORM_ID:
        case MSR_IA32_EBL_CR_POWERON:
        case MSR_IA32_DEBUGCTLMSR:
        case MSR_IA32_LASTBRANCHFROMIP:
        case MSR_IA32_LASTBRANCHTOIP:
        case MSR_IA32_LASTINTFROMIP:
        case MSR_IA32_LASTINTTOIP:
        case MSR_IA32_MISC_ENABLE:
            data = 0;
            break;

        case MSR_IA32_UCODE_REV:
            data = 0x100000000ULL;
            break;

        case MSR_P6_PERFCTR0:
        case MSR_P6_PERFCTR1:
        case MSR_P6_EVNTSEL0:
        case MSR_P6_EVNTSEL1:
        case MSR_IA32_PERF_GLOBAL_STATUS_SET:
            /* performance counters not supported. */
            data = 0;
            break;

        case 0xcd: /* fsb frequency */
            data = 3;
            break;

        case MSR_EBC_FREQUENCY_ID:
            data = 1 << 24;
            break;

        case MSR_IA32_APICBASE:
            data = vmm_lapic_get_base_msr(vcpu);
            break;

        default:
            DPRINTF(1, "rdmsr WARNING unsupported msr_no 0x%x\n", msr_no);
	    data = 0; /* should raise exception */
            break;

   }

    if (!ret) {
        vmm_set_user_context(&vcpu->guest_state, USER_CONTEXT_EAX, (uint32_t)(data & 0xffffffff));
        vmm_set_user_context(&vcpu->guest_state, USER_CONTEXT_EDX, (uint32_t)(data >> 32));
        vmm_guest_exit_next_instruction(&vcpu->guest_state, vcpu->guest_vcpu);
    }

    return ret;
}

int vmm_wrmsr_handler(vmm_vcpu_t *vcpu) {

    int ret = 0;

    unsigned int msr_no = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_ECX);
    unsigned int val_high = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EDX);
    unsigned int val_low = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EAX);

    DPRINTF(4, "wrmsr ecx 0x%x   value: 0x%x  0x%x\n", msr_no, val_high, val_low);

    // src reference: Linux kernel 3.11 kvm arch/x86/kvm/x86.c
    switch (msr_no) {
        case MSR_IA32_UCODE_REV:
        case MSR_IA32_UCODE_WRITE:
            break;

        case MSR_P6_PERFCTR0:
        case MSR_P6_PERFCTR1:
        case MSR_P6_EVNTSEL0:
        case MSR_P6_EVNTSEL1:
        case MSR_IA32_PERF_GLOBAL_STATUS_SET:
            /* performance counters not supported. */
            break;

        case MSR_IA32_APICBASE:
            vmm_lapic_set_base_msr(vcpu, val_low);
            break;

        default:
            DPRINTF(1, "wrmsr WARNING unsupported msr_no 0x%x\n", msr_no);
            ret = -1;
            break;
    }

    if (!ret)
        vmm_guest_exit_next_instruction(&vcpu->guest_state, vcpu->guest_vcpu);

    return ret;
}

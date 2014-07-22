/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/*handling msr read & write exceptions*/

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>

#include "vmm/config.h"
#include "vmm/vmm.h"
#include "vmm/helper.h"
#include "vmm/vmexit.h"
#include "vmm/processor/msr.h"


int vmm_rdmsr_handler(gcb_t *guest) {

    int ret = LIB_VMM_SUCC;
    unsigned int msr_no = guest->context.ecx;
    uint64_t data = 0;

    dprintf(4, "rdmsr ecx 0x%x\n", guest->context.ecx);

    // src reference: Linux kernel 3.11 kvm arch/x86/kvm/x86.c
    switch (msr_no) {
        case MSR_IA32_PLATFORM_ID:
        case MSR_IA32_EBL_CR_POWERON:
        case MSR_IA32_DEBUGCTLMSR:
        case MSR_IA32_LASTBRANCHFROMIP:
        case MSR_IA32_LASTBRANCHTOIP:
        case MSR_IA32_LASTINTFROMIP:
        case MSR_IA32_LASTINTTOIP:
            data = 0;
            break;

        case MSR_IA32_UCODE_REV: 
            data = 0x100000000ULL;
            break;

        case MSR_P6_PERFCTR0:
        case MSR_P6_PERFCTR1:
        case MSR_P6_EVNTSEL0:
        case MSR_P6_EVNTSEL1:
            /* performance counters not supported. */
            data = 0;
            break;
            
        case 0xcd: /* fsb frequency */
            data = 3;
            break;
        
        case MSR_EBC_FREQUENCY_ID:
            data = 1 << 24;
            break;

        default:
            dprintf(1, "rdmsr WARNING unsupported msr_no 0x%x\n", msr_no);
            ret = LIB_VMM_ERR;
            break;

   }

    if (ret == LIB_VMM_SUCC) {
        vmm_write_dword_reg(data, &guest->context.eax, &guest->context.edx);
        guest->context.eip += guest->instruction_length;
    }

    return ret;
}


int vmm_wrmsr_handler(gcb_t *guest) {

    int ret = LIB_VMM_SUCC;

    unsigned int msr_no = guest->context.ecx;
    
    dprintf(4, "wrmsr ecx 0x%x   value: 0x%x  0x%x\n", guest->context.ecx, guest->context.edx, guest->context.eax);

    // src reference: Linux kernel 3.11 kvm arch/x86/kvm/x86.c
    switch (msr_no) {
        case MSR_IA32_UCODE_REV:
        case MSR_IA32_UCODE_WRITE:
            break;

        case MSR_P6_PERFCTR0:
        case MSR_P6_PERFCTR1:
        case MSR_P6_EVNTSEL0:
        case MSR_P6_EVNTSEL1:
            /* performance counters not supported. */
            break;

        default:
            dprintf(1, "wrmsr WARNING unsupported msr_no 0x%x\n", msr_no);
            ret = LIB_VMM_ERR;
            break;
    }

    if (ret == LIB_VMM_SUCC)
        guest->context.eip += guest->instruction_length;

    return ret;
}

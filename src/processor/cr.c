/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/*vm exit handler for control register access*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>

#include "vmm/config.h"
#include "vmm/vmcs.h"

#include "vmm/vmm.h"

#include "vmm/vmexit.h"
#include "vmm/helper.h"
#include "vmm/platform/sys.h"

static int vmm_cr_set_cr0(gcb_t *guest, unsigned int value) {

    if (value & CR0_RESERVED_BITS)
        return LIB_VMM_ERR;

    /*read the cr0 value*/
    int set = vmm_vmcs_read(LIB_VMM_VCPU_CAP, VMX_GUEST_CR0);

    dprintf(4, "cr0 val 0x%x guest set 0x%x\n", set, value);

    /*set the cr0 value with the shadow value*/
    value |= (guest->cr0_mask & guest->cr0_shadow);

    if (value == set) 
        return LIB_VMM_SUCC;

    vmm_vmcs_write(LIB_VMM_VCPU_CAP, VMX_GUEST_CR0, value);

    return LIB_VMM_SUCC;
}

static int vmm_cr_set_cr3(gcb_t *guest, unsigned int value) {
    
    static int init_flag = 0;
    
    /*guest os trying to reset the page table structure*/
    int set = vmm_vmcs_read(LIB_VMM_VCPU_CAP, VMX_GUEST_CR3);

    dprintf(4, "cr3 val 0x%x guest set 0x%x\n", set, value);

    vmm_vmcs_write(LIB_VMM_VCPU_CAP, VMX_GUEST_CR3, value);

    /*cancel the big page set by platform/boot.c & vmcs_init()*/
    if (!init_flag) {
        guest->cr4_mask &= ~X86_CR4_PSE;
        guest->cr4_shadow &= ~X86_CR4_PSE;

        set = vmm_vmcs_read(LIB_VMM_VCPU_CAP, VMX_GUEST_CR4);
        value = set & (~X86_CR4_PSE);

        if (set & X86_CR4_PSE)
            vmm_vmcs_write(LIB_VMM_VCPU_CAP, VMX_GUEST_CR4, set & (~X86_CR4_PSE));
       
        dprintf(4, "cr4 val 0x%x guest set 0x%x\n", set, value);

        init_flag = 1;
    
    }
    return LIB_VMM_SUCC;
}

static int vmm_cr_get_cr3(gcb_t *guest, unsigned int *value) {

    *value = guest->cr3;
    
    return LIB_VMM_SUCC;

}


static int vmm_cr_set_cr4(gcb_t *guest, unsigned int value) {
    
    if (value & CR4_RESERVED_BITS)
        return LIB_VMM_ERR;

    int set = vmm_vmcs_read(LIB_VMM_VCPU_CAP, VMX_GUEST_CR4);

    dprintf(4, "cr4 val 0x%x guest set 0x%x\n", set, value);
       
    /*set the cr0 value with the shadow value*/
    value |= (guest->cr4_shadow & guest->cr4_mask);
    
    if (set == value)
        return LIB_VMM_SUCC;

    vmm_vmcs_write(LIB_VMM_VCPU_CAP, VMX_GUEST_CR4, value);

    return LIB_VMM_SUCC;
}


static int vmm_cr_clts(gcb_t *guest) {

    return LIB_VMM_ERR;
}

static int vmm_cr_lmsw(gcb_t *guest, unsigned int value) {

    return LIB_VMM_ERR;

}


int vmm_cr_access_handler(gcb_t *guest) {

    unsigned int exit_qualification, val;
    int cr, reg, ret = LIB_VMM_ERR;

    exit_qualification = guest->qualification;
    cr = exit_qualification & 15;
    reg = (exit_qualification >> 8) & 15;

    vm_exit_copyin_regs(guest);

    switch ((exit_qualification >> 4) & 3) {
        case 0: /* mov to cr */
            val = guest->regs[reg];

            switch (cr) {
                case 0:
                    ret = vmm_cr_set_cr0(guest, val);
                    break;
                case 3:
                    ret = vmm_cr_set_cr3(guest, val);
                    break;
                case 4:
                    ret = vmm_cr_set_cr4(guest, val);
                    break;
                case 8: 

#if 0

                    u8 cr8_prev = kvm_get_cr8(vcpu);
                    u8 cr8 = kvm_register_read(vcpu, reg);
                    err = kvm_set_cr8(vcpu, cr8);
                    kvm_complete_insn_gp(vcpu, err);
                    if (irqchip_in_kernel(vcpu->kvm))
                        return 1;
                    if (cr8_prev <= cr8)
                        return 1;
                    vcpu->run->exit_reason = KVM_EXIT_SET_TPR;
                    return 0;
#endif

                default: 
                    dprintf(4, "unhandled control register: op %d cr %d\n",
                            (int)(exit_qualification >> 4) & 3, cr);
                    break;

            }
            break;

        case 1: /*mov from cr*/
            switch (cr) {
                case 3:

                    ret = vmm_cr_get_cr3(guest, &val);
                    if (ret == LIB_VMM_SUCC)
                        guest->regs[reg] = val;

                    break;
                case 8:
#if 0
                    val = kvm_get_cr8(vcpu);
                    kvm_register_write(vcpu, reg, val);
                    trace_kvm_cr_read(cr, val);
                    skip_emulated_instruction(vcpu);
                    return 1;
#endif
                default:
                    dprintf(4, "unhandled control register: op %d cr %d\n",
                            (int)(exit_qualification >> 4) & 3, cr);
                    break;

            }
            break;
        case 2: /* clts */
            ret = vmm_cr_clts(guest);
            break;

        case 3: /* lmsw */
            val = (exit_qualification >> LMSW_SOURCE_DATA_SHIFT) & 0x0f;
            ret = vmm_cr_lmsw(guest, val);
            break;
        
        default:
            dprintf(4, "unhandled control register: op %d cr %d\n",
                    (int)(exit_qualification >> 4) & 3, cr);
            break;
    }

    if (ret == LIB_VMM_SUCC) {
        vm_exit_copyout_regs(guest);
        guest->context.eip += guest->instruction_length;
    }

    return ret;
}




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

#include <stdio.h>
#include <stdlib.h>

#include <sel4/sel4.h>
#include <utils/util.h>

#include "vmm/debug.h"
#include "vmm/processor/platfeature.h"
#include "vmm/platform/vmcs.h"

#include "vmm/vmm.h"

static int vmm_cr_set_cr0(vmm_t *vmm, unsigned int value) {

    if (value & CR0_RESERVED_BITS)
        return -1;

    /*read the cr0 value*/
    int set = vmm_guest_state_get_cr0(&vmm->guest_state, vmm->guest_vcpu);

    DPRINTF(4, "cr0 val 0x%x guest set 0x%x\n", set, value);

    /*set the cr0 value with the shadow value*/
    value |= (vmm->guest_state.virt.cr.cr0_mask & vmm->guest_state.virt.cr.cr0_shadow);

    if (value == set) 
        return 0;

    vmm_guest_state_set_cr0(&vmm->guest_state, value);

    return 0;
}

static int vmm_cr_set_cr3(vmm_t *vmm, unsigned int value) {
    assert(!"Should not get cr3 access");
    return -1;
}

static int vmm_cr_get_cr3(vmm_t *vmm, unsigned int *value) {
    *value = vmm_guest_state_get_cr3(&vmm->guest_state, vmm->guest_vcpu);
    return 0;

}


static int vmm_cr_set_cr4(vmm_t *vmm, unsigned int value) {

    if (value & CR4_RESERVED_BITS)
        return -1;

    int set = vmm_guest_state_get_cr4(&vmm->guest_state, vmm->guest_vcpu);

    DPRINTF(4, "cr4 val 0x%x guest set 0x%x\n", set, value);

    /*set the cr0 value with the shadow value*/
    value |= (vmm->guest_state.virt.cr.cr4_shadow & vmm->guest_state.virt.cr.cr4_mask);

    if (set == value)
        return 0;

    vmm_guest_state_set_cr4(&vmm->guest_state, value);

    return 0;
}


static int vmm_cr_clts(vmm_t *vmm) {
    LOG_INFO("Ignoring call of clts");

    return -1;
}

static int vmm_cr_lmsw(vmm_t *vmm, unsigned int value) {
    LOG_INFO("Ignoring call of lmsw");

    return -1;

}

/* Convert exit regs to seL4 user context */
static int crExitRegs[] = {
    USER_CONTEXT_EAX,
    USER_CONTEXT_ECX,
    USER_CONTEXT_EDX,
    USER_CONTEXT_EBX,
    USER_CONTEXT_ESP,
    USER_CONTEXT_EBP,
    USER_CONTEXT_ESI,
    USER_CONTEXT_EDI
};

int vmm_cr_access_handler(vmm_t *vmm) {

    unsigned int exit_qualification, val;
    int cr, reg, ret = -1;

    exit_qualification = vmm_guest_exit_get_qualification(&vmm->guest_state);
    cr = exit_qualification & 15;
    reg = (exit_qualification >> 8) & 15;

    switch ((exit_qualification >> 4) & 3) {
        case 0: /* mov to cr */
            val = vmm_read_user_context(&vmm->guest_state, crExitRegs[reg]);

            switch (cr) {
                case 0:
                    ret = vmm_cr_set_cr0(vmm, val);
                    break;
                case 3:
                    ret = vmm_cr_set_cr3(vmm, val);
                    break;
                case 4:
                    ret = vmm_cr_set_cr4(vmm, val);
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
                    DPRINTF(4, "unhandled control register: op %d cr %d\n",
                            (int)(exit_qualification >> 4) & 3, cr);
                    break;

            }
            break;

        case 1: /*mov from cr*/
            switch (cr) {
                case 3:

                    ret = vmm_cr_get_cr3(vmm, &val);
                    if (!ret)
                        vmm_set_user_context(&vmm->guest_state, crExitRegs[reg], val);

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
                    DPRINTF(4, "unhandled control register: op %d cr %d\n",
                            (int)(exit_qualification >> 4) & 3, cr);
                    break;

            }
            break;
        case 2: /* clts */
            ret = vmm_cr_clts(vmm);
            break;

        case 3: /* lmsw */
            val = (exit_qualification >> LMSW_SOURCE_DATA_SHIFT) & 0x0f;
            ret = vmm_cr_lmsw(vmm, val);
            break;
        
        default:
            DPRINTF(4, "unhandled control register: op %d cr %d\n",
                    (int)(exit_qualification >> 4) & 3, cr);
            break;
    }

    if (!ret) {
        vmm_guest_exit_next_instruction(&vmm->guest_state);
    }

    return ret;
}

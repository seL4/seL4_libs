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

/*vm exit handler for control register access*/

#include <stdio.h>
#include <stdlib.h>

#include <sel4/sel4.h>
#include <utils/util.h>

#include "vmm/debug.h"
#include "vmm/processor/platfeature.h"
#include "vmm/platform/vmcs.h"

#include "vmm/vmm.h"

static inline unsigned int apply_cr_bits(unsigned int cr, unsigned int mask, unsigned int host_bits) {
    /* force any bit in the mask to be the value from the shadow (both enabled and disabled) */
    cr |= (mask & host_bits);
    cr &= ~(mask & (~host_bits));
    return cr;
}

static int vmm_cr_set_cr0(vmm_vcpu_t *vcpu, unsigned int value) {

    if (value & CR0_RESERVED_BITS)
        return -1;

    /* check if paging is being enabled */
    if ((value & X86_CR0_PG) && !(vcpu->guest_state.virt.cr.cr0_shadow & X86_CR0_PG)) {
        /* guest is taking over paging. So we can no longer care about some of our CR4 values, and
         * we don't need cr3 load/store exiting anymore */
        unsigned int new_mask = vcpu->guest_state.virt.cr.cr4_mask & ~(X86_CR4_PSE | X86_CR4_PAE);
        unsigned int cr4_value = vmm_guest_state_get_cr4(&vcpu->guest_state, vcpu->guest_vcpu);
        /* for any bits that have changed in the mask, grab them from the shadow */
        cr4_value = apply_cr_bits(cr4_value, new_mask ^ vcpu->guest_state.virt.cr.cr4_mask, vcpu->guest_state.virt.cr.cr4_shadow);
        /* update mask and cr4 value */
        vcpu->guest_state.virt.cr.cr4_mask = new_mask;
        vmm_vmcs_write(vcpu->guest_vcpu, VMX_CONTROL_CR4_MASK, new_mask);
        vmm_guest_state_set_cr4(&vcpu->guest_state, cr4_value);
        /* now turn of cr3 load/store exiting */
        unsigned int ppc = vmm_guest_state_get_control_ppc(&vcpu->guest_state);
        ppc &= ~(VMX_CONTROL_PPC_CR3_LOAD_EXITING | VMX_CONTROL_PPC_CR3_STORE_EXITING);
        vmm_guest_state_set_control_ppc(&vcpu->guest_state, ppc);
        /* load the cached cr3 value */
        vmm_guest_state_set_cr3(&vcpu->guest_state, vcpu->guest_state.virt.cr.cr3_guest);
    }

    /* check if paging is being disabled */
    if (!(value & X86_CR0_PG) && (vcpu->guest_state.virt.cr.cr0_shadow & X86_CR0_PG)) {
        ZF_LOGE("Do not support paging being disabled");
        /* if we did support disabling paging we should re-enable cr3 load/store exiting,
         * watch the pae bit in cr4 again and then */
        return -1;
    }

    /* update the guest shadow */
    vcpu->guest_state.virt.cr.cr0_shadow = value;
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_CONTROL_CR0_READ_SHADOW, value);

    value = apply_cr_bits(value, vcpu->guest_state.virt.cr.cr0_mask, vcpu->guest_state.virt.cr.cr0_host_bits);

    vmm_guest_state_set_cr0(&vcpu->guest_state, value);

    return 0;
}

static int vmm_cr_set_cr3(vmm_vcpu_t *vcpu, unsigned int value) {
    /* if the guest hasn't turned on paging then just cache this */
    vcpu->guest_state.virt.cr.cr3_guest = value;
    if (vcpu->guest_state.virt.cr.cr0_shadow & X86_CR0_PG) {
        vmm_guest_state_set_cr3(&vcpu->guest_state, value);
    }
    return 0;
}

static int vmm_cr_get_cr3(vmm_vcpu_t *vcpu, unsigned int *value) {
    if (vcpu->guest_state.virt.cr.cr0_shadow & X86_CR0_PG) {
        *value = vmm_guest_state_get_cr3(&vcpu->guest_state, vcpu->guest_vcpu);
    } else {
        *value = vcpu->guest_state.virt.cr.cr3_guest;
    }
    return 0;

}

static int vmm_cr_set_cr4(vmm_vcpu_t *vcpu, unsigned int value) {

    if (value & CR4_RESERVED_BITS)
        return -1;

    /* update the guest shadow */
    vcpu->guest_state.virt.cr.cr4_shadow = value;
    vmm_vmcs_write(vcpu->guest_vcpu, VMX_CONTROL_CR4_READ_SHADOW, value);

    value = apply_cr_bits(value, vcpu->guest_state.virt.cr.cr4_mask, vcpu->guest_state.virt.cr.cr4_host_bits);

    vmm_guest_state_set_cr4(&vcpu->guest_state, value);

    return 0;
}

static int vmm_cr_clts(vmm_vcpu_t *vcpu) {
    ZF_LOGI("Ignoring call of clts");

    return -1;
}

static int vmm_cr_lmsw(vmm_vcpu_t *vcpu, unsigned int value) {
    ZF_LOGI("Ignoring call of lmsw");

    return -1;

}

/* Convert exit regs to seL4 user context */
static int crExitRegs[] = {
    USER_CONTEXT_EAX,
    USER_CONTEXT_ECX,
    USER_CONTEXT_EDX,
    USER_CONTEXT_EBX,
    /*USER_CONTEXT_ESP*/-1,
    USER_CONTEXT_EBP,
    USER_CONTEXT_ESI,
    USER_CONTEXT_EDI
};

int vmm_cr_access_handler(vmm_vcpu_t *vcpu) {

    unsigned int exit_qualification, val;
    int cr, reg, ret = -1;

    exit_qualification = vmm_guest_exit_get_qualification(&vcpu->guest_state);
    cr = exit_qualification & 15;
    reg = (exit_qualification >> 8) & 15;

    switch ((exit_qualification >> 4) & 3) {
        case 0: /* mov to cr */
            if (crExitRegs[reg] < 0) {
                return -1;
            }
            val = vmm_read_user_context(&vcpu->guest_state, crExitRegs[reg]);

            switch (cr) {
                case 0:
                    ret = vmm_cr_set_cr0(vcpu, val);
                    break;
                case 3:
                    ret = vmm_cr_set_cr3(vcpu, val);
                    break;
                case 4:
                    ret = vmm_cr_set_cr4(vcpu, val);
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

                    ret = vmm_cr_get_cr3(vcpu, &val);
                    if (!ret)
                        vmm_set_user_context(&vcpu->guest_state, crExitRegs[reg], val);

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
            ret = vmm_cr_clts(vcpu);
            break;

        case 3: /* lmsw */
            val = (exit_qualification >> LMSW_SOURCE_DATA_SHIFT) & 0x0f;
            ret = vmm_cr_lmsw(vcpu, val);
            break;

        default:
            DPRINTF(4, "unhandled control register: op %d cr %d\n",
                    (int)(exit_qualification >> 4) & 3, cr);
            break;
    }

    if (!ret) {
        vmm_guest_exit_next_instruction(&vcpu->guest_state, vcpu->guest_vcpu);
    }

    return ret;
}

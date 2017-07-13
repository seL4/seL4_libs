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

#include "vmm/vmm.h"
#include "vmm/debug.h"
#include "vmm/vmcall.h"

static vmcall_handler_t *get_handle(vmm_t *vmm, int token);

static vmcall_handler_t *get_handle(vmm_t *vmm, int token) {
    int i;
    for(i = 0; i < vmm->vmcall_num_handlers; i++) {
        if(vmm->vmcall_handlers[i].token == token) {
            return &vmm->vmcall_handlers[i];
        }
    }

    return NULL;
}

int reg_new_handler(vmm_t *vmm, vmcall_handler func, int token) {
    unsigned int *hnum = &(vmm->vmcall_num_handlers);
    if(get_handle(vmm, token) != NULL) {
        return -1;
    }

    vmm->vmcall_handlers = realloc(vmm->vmcall_handlers, sizeof(vmcall_handler_t) * (*hnum + 1));
    if(vmm->vmcall_handlers == NULL) {
        return -1;
    }

    vmm->vmcall_handlers[*hnum].func = func;
    vmm->vmcall_handlers[*hnum].token = token;
    vmm->vmcall_num_handlers++;

    DPRINTF(4, "Reg. handler %u for vmm, total = %u\n", *hnum - 1, *hnum);
    return 0;
}

int vmm_vmcall_handler(vmm_vcpu_t *vcpu) {
    int res;
    vmcall_handler_t *h;
    int token = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EAX);
    h = get_handle(vcpu->vmm, token);
    if(h == NULL) {
        DPRINTF(2, "Failed to find handler for token:%x\n", token);
        vmm_guest_exit_next_instruction(&vcpu->guest_state, vcpu->guest_vcpu);
        return 0;
    }

    res = h->func(vcpu);
    if(res == 0) {
        vmm_guest_exit_next_instruction(&vcpu->guest_state, vcpu->guest_vcpu);
    }

    return res;
}

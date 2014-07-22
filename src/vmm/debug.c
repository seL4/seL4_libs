/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/* Debugging helper functions used by VMM lib.
 *     Authors:
 *         Qian Ge
 */

#include <stdio.h>
#include <stdlib.h>

#include <sel4/sel4.h>

#include "vmm/debug.h"

/* Print out the context of a guest OS thread. */
void vmm_print_guest_context(int level, vmm_t *vmm) {
    DPRINTF(level, "================== GUEST OS CONTEXT =================\n");

    DPRINTF(level, "exit info : reason 0x%x    qualification 0x%x   instruction len 0x%x\n",
                    vmm_guest_exit_get_reason(&vmm->guest_state), vmm_guest_exit_get_qualification(&vmm->guest_state), vmm_guest_exit_get_int_len(&vmm->guest_state));
    DPRINTF(level, "            guest physical 0x%x     rflags 0x%x \n",
                   vmm_guest_exit_get_physical(&vmm->guest_state), vmm_guest_state_get_rflags(&vmm->guest_state, vmm->guest_vcpu));
    DPRINTF(level, "            guest interruptibility 0x%x   control entry 0x%x\n",
                   vmm_guest_state_get_interruptibility(&vmm->guest_state, vmm->guest_vcpu), vmm_guest_state_get_control_entry(&vmm->guest_state, vmm->guest_vcpu));

    DPRINTF(level, "eip 0x%8x         esp 0x%8x      eflags 0x%8x\n",
                   vmm_read_user_context(&vmm->guest_state, USER_CONTEXT_EIP), vmm_read_user_context(&vmm->guest_state, USER_CONTEXT_ESP), vmm_read_user_context(&vmm->guest_state, USER_CONTEXT_EFLAGS));
    DPRINTF(level, "eax 0x%8x         ebx 0x%8x      ecx 0x%8x\n",
                   vmm_read_user_context(&vmm->guest_state, USER_CONTEXT_EAX), vmm_read_user_context(&vmm->guest_state, USER_CONTEXT_EBX), vmm_read_user_context(&vmm->guest_state, USER_CONTEXT_ECX));
    DPRINTF(level, "edx 0x%8x         esi 0x%8x      edi 0x%8x\n",
                   vmm_read_user_context(&vmm->guest_state, USER_CONTEXT_EDX), vmm_read_user_context(&vmm->guest_state, USER_CONTEXT_ESI), vmm_read_user_context(&vmm->guest_state, USER_CONTEXT_EDI));
    DPRINTF(level, "ebp 0x%8x         tls_base 0x%8x      fs 0x%8x\n",
                   vmm_read_user_context(&vmm->guest_state, USER_CONTEXT_EBP), vmm_read_user_context(&vmm->guest_state, USER_CONTEXT_TLS_BASE), vmm_read_user_context(&vmm->guest_state, USER_CONTEXT_FS));
    DPRINTF(level, "gs 0x%8x \n", vmm_read_user_context(&vmm->guest_state, USER_CONTEXT_GS));

    DPRINTF(level, "cr0 0x%x      cr3 0x%x   cr4 0x%x\n", vmm_guest_state_get_cr0(&vmm->guest_state, vmm->guest_vcpu), vmm_guest_state_get_cr3(&vmm->guest_state, vmm->guest_vcpu), vmm_guest_state_get_cr4(&vmm->guest_state, vmm->guest_vcpu));

}

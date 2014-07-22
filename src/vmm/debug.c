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

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <sel4/sel4.h>
#include "vmm/config.h"
#include "vmm/vmm.h"
#include "vmm/vmexit.h"

extern gcb_t vmm_gc;

static exit_reasons_table_t vmm_exit_reasons[] = { VMX_EXIT_REASONS };  

static inline const char *vmm_get_exit_reason (unsigned int exit_code) {
    unsigned int i = 0;
    while (i != VMM_EXIT_REASON_NUM) {
       if (vmm_exit_reasons[i].exit_code == exit_code) {
           return vmm_exit_reasons[i].reason;
       }
       i++;
    }
    return NULL;
}

/* Print out the context of a guest OS thread. */
void vmm_print_guest_context(int level) {

    dprintf(level, "================== GUEST OS CONTEXT =================\n");

    dprintf(level, "exit info : reason 0x%x    qualification 0x%x   instruction len 0x%x\n",
                    vmm_gc.reason, vmm_gc.qualification, vmm_gc.instruction_length);
    dprintf(level, "            guest physical 0x%x     rflags 0x%x \n",
                   vmm_gc.guest_physical, vmm_gc.rflags);
    dprintf(level, "            guest interruptibility 0x%x   control entry 0x%x\n",
                   vmm_gc.guest_interruptibility, vmm_gc.control_entry);

    dprintf(level, "eip 0x%8x         esp 0x%8x      eflags 0x%8x\n",
                   vmm_gc.context.eip, vmm_gc.context.esp, vmm_gc.context.eflags);
    dprintf(level, "eax 0x%8x         ebx 0x%8x      ecx 0x%8x\n",
                   vmm_gc.context.eax, vmm_gc.context.ebx, vmm_gc.context.ecx);
    dprintf(level, "edx 0x%8x         esi 0x%8x      edi 0x%8x\n",
                   vmm_gc.context.edx, vmm_gc.context.esi, vmm_gc.context.edi);
    dprintf(level, "ebp 0x%8x         tls_base 0x%8x      fs 0x%8x\n",
                   vmm_gc.context.ebp, vmm_gc.context.tls_base, vmm_gc.context.fs);
    dprintf(level, "gs 0x%8x \n", vmm_gc.context.gs);

    dprintf(level, "cr0 0x%x      cr3 0x%x   cr4 0x%x\n", vmm_gc.cr0, vmm_gc.cr3, vmm_gc.cr4);

    dprintf(level, "exit reason: %s \n", vmm_get_exit_reason(vmm_gc.reason));

}

/* Print out the IPC buffer content. */
void vmm_print_ipc_msg(seL4_Word msg_len) {
    dprintf(3, "msg content: \n");
    for (int i = 0; i < msg_len; i++) {
        dprintf(3, "%d:    0x%x\n", i, seL4_GetMR(i));
    }
    dprintf(3, "\n");
}


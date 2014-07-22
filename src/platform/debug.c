/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/*the debuging functions for platfom module*/
#include <stdio.h>
#include <stdint.h>

#include <sel4/sel4.h>
#include "vmm/config.h"
#include "vmm/platform/sys.h"
#include "vmm/helper.h"

extern plat_net_t vmm_plat_net;

/* Wait for fault msg from other host threads. */
void vmm_plat_fault_wait() {
    seL4_MessageInfo_t msg;
    seL4_Word sender;
    unsigned int msg_len;

    printf("plat: wait for fault msg\n");

    while (1) {
        msg = seL4_Wait(vmm_plat_net.fault_ep, &sender);
        msg_len = seL4_MessageInfo_get_length(msg);

        if (seL4_MessageInfo_get_label(msg) == seL4_VMFault) {
            printf(COLOUR_R "plat: ##### ERROR ::: VMM HOST THREAD CRASHED!!! #####\n"
                   COLOUR_RESET);
            printf(COLOUR_R "   >>> " COLOUR_RESET "Thread Badge: %d\n", sender);
            printf(COLOUR_R "   >>> " COLOUR_RESET "Program Counter: 0x%x\n", seL4_GetMR(0));
            printf(COLOUR_R "   >>> " COLOUR_RESET "Fault Address: 0x%x\n", seL4_GetMR(1));
            printf(COLOUR_R "   >>> " COLOUR_RESET "Fault Type: %s\n",
                   seL4_GetMR(2) ? "Instruction" : "Data");
            printf(COLOUR_R "   >>> " COLOUR_RESET "Fault Status Register: 0x%x\n", seL4_GetMR(3));
            panic("VMM host thread crashed.");
        }

        printf(COLOUR_Y "plat: WARNING recv msg from %d info 0x%x\n" COLOUR_RESET,
               sender, msg.words[0]);

        for (int i = 0; i < msg_len; i++) {
            printf("        MR%d = 0x%x \n", i, seL4_GetMR(i));
        }
    }

}

/* Print out thread context. */
void vmm_plat_print_thread_context(char *name, seL4_UserContext *context) {
    printf("--------------%s  Context----------------------\n", name);
    printf("eip 0x%8x         esp 0x%8x      eflags 0x%8x\n",
           context->eip, context->esp, context->eflags);
    printf("eax 0x%8x         ebx 0x%8x      ecx 0x%8x\n",
           context->eax, context->ebx, context->ecx);
    printf("edx 0x%8x         esi 0x%8x      edi 0x%8x\n",
           context->edx, context->esi, context->edi);
    printf("ebp 0x%8x         tls_base 0x%8x      fs 0x%8x\n",
           context->ebp, context->tls_base, context->fs);
    printf("gs 0x%8x \n", context->gs);
}


/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */
/* Interrupt management driver.
 *     Authors:
 *         Adrian Danis
 */

#include <assert.h>
#include <stdio.h>
#include <sel4/sel4.h>
#include <sel4utils/util.h>

#include "vmm/config.h"
#include "vmm/vmm.h"
#include "vmm/io.h"
#include "vmm/helper.h"
#include "vmm/driver/int_manger.h"
#include "vmm/driver/i8259.h"

/* The entry function for interrupt manager thread. */
void vmm_driver_interrupt_manager_run(void) {
    seL4_MessageInfo_t msg;
    seL4_Word sender;
    seL4_Word label;
    unsigned int len, reply_len = 0; 

    dprintf(2, "INTERRUPT MANAGER STARTED\n");

    /* Initialise the PIC emulator. */
    vmm_pic_init_state();

    while (1) {
        msg = seL4_Wait(LIB_VMM_DRIVER_INTERRUPT_CAP, &sender);  
        len = seL4_MessageInfo_get_length(msg);
        label = seL4_MessageInfo_get_label(msg);
        
        dprintf(3, "vmm intmgr : interrupt manager got a message.\n");
        reply_len = 0;

        if (sender & LIB_VMM_DRIVER_ASYNC_MESSAGE_BADGE) {

            /* Async message - assume it means that we have recieved an interrupt from the seL4
             * kernel.
             */
            uint32_t irqs = sender ^ LIB_VMM_DRIVER_ASYNC_MESSAGE_BADGE;
            while(irqs) {
                int irq = 31 - CLZ(irqs);
                irqs ^= BIT(irq);
                assert(BIT(irq) != LIB_VMM_DRIVER_ASYNC_MESSAGE_BADGE);
                /* When we assigned the badges we had to skip the bit used to indicate async
                 * delivery. So need correct the IRQ calculation here. */
                if (BIT(irq) > LIB_VMM_DRIVER_ASYNC_MESSAGE_BADGE) {
                    irq--;
                }
                /* Inject irq. Assume guest 0 at the moment. */
                /* TODO: This should be extended to not assume guest 0. */
                vmm_pic_set_irq(LIB_VMM_GUEST_OS_BADGE_START, irq, irq, 1);
                vmm_pic_set_irq(LIB_VMM_GUEST_OS_BADGE_START, irq, irq, 0);
            }

            /* PIC emulator manages acknowledgement of IRQs, so we don't need to do anything here */
        } else {

            /* Sync message - means this is an IPC message from a host thread. */
            switch(label) {
            case LABEL_VMM_GUEST_IO_MESSAGE: {
                /* An IO message for the PIC controller. */
                io_msg_t recv;
                vmm_get_msg(&recv, len);
                reply_len = vmm_pic_msg_proc(recv.sender, len);
                break;
            }
            case LABEL_INT_MAN_GET_INTERRUPT: {
                /* Host communication to get current guest interrupt state. */
                int s = seL4_GetMR(0);
                reply_len = 1;
                if (vmm_pic_has_irq(s)) {
                    int irq = vmm_pic_read_irq(s);
                    seL4_SetMR(0, irq);
                } else {
                    seL4_SetMR(0, -1);
                }
                break;
            }
            case LABEL_INT_MAN_HAS_INTERRUPT: {
                /* Host communication to get current guest interrupt state. */
                int s = seL4_GetMR(0);
                reply_len = 1;
                int has = vmm_pic_has_irq(s);
                seL4_SetMR(0, has);
                break;
            }
            default:
                assert(!"Received unknown message");
            }

            /* Reply to IPC. */
            seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, reply_len);
            seL4_Reply(reply);
        }
    }

}



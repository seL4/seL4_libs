/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef _LIB_VMM_INTERRUPT_H_
#define _LIB_VMM_INTERRUPT_H_

/* Handler for a guest exit that is triggered when the guest is
 * ready to receive interrupts */

int vmm_pending_interrupt_handler(struct vmm *vmm);

/* Request that the guest exist as soon as it is ready to receive
 * interrupts */
void wait_for_guest_ready(struct vmm *vmm);

#endif

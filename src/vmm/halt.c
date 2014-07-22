/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/*vm exits related with hlt'ing*/

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <sel4/sel4.h>

#include "vmm/config.h"
#include "vmm/vmm.h"
#include "vmm/vmexit.h"
#include "vmm/vmcs.h"

/* Handling EPT violation VMExit Events. */
int vmm_hlt_handler(gcb_t *guest) {
    if (!(vmm_vmcs_read(LIB_VMM_VCPU_CAP, VMX_GUEST_RFLAGS) & BIT(9))) {
        printf("Halted forever :(\n");
    }
    if (!interrupt_pending(guest)) {
        guest->interrupt_halt = 1;
    }
    guest->context.eip += guest->instruction_length;
    return LIB_VMM_SUCC;
}

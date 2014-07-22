/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/*vm exits related with ept violations*/

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <sel4/sel4.h>

#include "vmm/config.h"
#include "vmm/vmm.h"
#include "vmm/vmexit.h"
#include "vmm/vmcs.h"

/* Handling EPT violation VMExit Events. */
int vmm_ept_violation_handler(gcb_t *guest) {
    /* Read linear address that guest is trying to access. */
    unsigned int linear_address = vmm_vmcs_read(LIB_VMM_VCPU_CAP, VMX_DATA_GUEST_LINEAR_ADDRESS);
    printf(COLOUR_R "!!!!!!!! ALERT :: GUEST OS PAGE FAULT !!!!!!!!\n");
    printf("    Guest OS 0x%x VMExit due to EPT Violation:\n", guest->sender);
    printf("        Linear address 0x%x.\n", linear_address);
    printf("        Guest-Physical address 0x%x.\n", guest->guest_physical);
    printf("        Instruction pointer 0x%x.\n", guest->context.eip);
    printf("    This is most likely due to a bug or misconfiguration.\n");          
    printf("    The faulting Guest OS thread will now be blocked forever.\n" COLOUR_RESET);
#ifdef CONFIG_VMM_IGNORE_EPT_VIOLATION
    guest->context.eip += guest->instruction_length;
    return LIB_VMM_SUCC;
#else
    return LIB_VMM_ERR;
#endif
}

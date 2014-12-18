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
#include <stdlib.h>

#include <sel4/sel4.h>

#include "vmm/debug.h"
#include "vmm/vmm.h"
#include "vmm/platform/vmcs.h"
#include "vmm/mmio.h"

/* Handling EPT violation VMExit Events. */
int vmm_ept_violation_handler(vmm_t *vmm) {

	uintptr_t guest_phys = vmm_guest_exit_get_physical(&vmm->guest_state);
	unsigned int qualification = vmm_guest_exit_get_qualification(&vmm->guest_state);

	int e = vmm_mmio_exit_handler(vmm, guest_phys, qualification);

	if (e == 0) {
		DPRINTF(0, "EPT violation handled by mmio\n");
	} else {
		/* Read linear address that guest is trying to access. */
		unsigned int linear_address = vmm_vmcs_read(vmm->guest_vcpu, VMX_DATA_GUEST_LINEAR_ADDRESS);
		printf(COLOUR_R "!!!!!!!! ALERT :: GUEST OS PAGE FAULT !!!!!!!!\n");
		printf("    Guest OS VMExit due to EPT Violation:\n");
		printf("        Linear address 0x%x.\n", linear_address);
		printf("        Guest-Physical address 0x%x.\n", vmm_guest_exit_get_physical(&vmm->guest_state));
		printf("        Instruction pointer 0x%x.\n", vmm_read_user_context(&vmm->guest_state, USER_CONTEXT_EIP));
		printf("    This is most likely due to a bug or misconfiguration.\n" COLOUR_RESET);
	}

#ifndef CONFIG_VMM_IGNORE_EPT_VIOLATION
    printf(COLOUR_R "    The faulting Guest OS thread will now be blocked forever.\n" COLOUR_RESET);
    return -1;
#else
    vmm_guest_exit_next_instruction(&vmm->guest_state);
    return 0;
#endif
}

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef _LIB_VMM_VMEXIT_H_
#define _LIB_VMM_VMEXIT_H_

#include <vmm/interrupt.h>

typedef int(*vmexit_handler_ptr)(struct vmm *vmm);

/*vm exit handlers*/
int vmm_cpuid_handler(struct vmm *vmm);
int vmm_ept_violation_handler(struct vmm *vmm);
int vmm_wrmsr_handler(struct vmm *vmm);
int vmm_rdmsr_handler(struct vmm *vmm);
int vmm_io_instruction_handler(struct vmm *vmm);
int vmm_hlt_handler(struct vmm *vmm);
int vmm_vmx_timer_handler(struct vmm *vmm);
int vmm_cr_access_handler(struct vmm *vmm);

#endif

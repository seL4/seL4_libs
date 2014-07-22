/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef _LIB_VMM_VMCS_H_
#define _LIB_VMM_VMCS_H_

#include <sel4/sel4.h>

int vmm_vmcs_read(seL4_CPtr vcpu, unsigned int field);
void vmm_vmcs_write(seL4_CPtr vcpu, unsigned int field, unsigned int value); 
void vmm_vmcs_init_guest(struct vmm *vmm);

#endif /* _LIB_VMM_VMCS_H_ */

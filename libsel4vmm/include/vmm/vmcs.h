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

#include <vmm/vmm.h>

int vmm_vmcs_read(seL4_CPtr vcpu, seL4_Word field);
void vmm_vmcs_write(seL4_CPtr vcpu, seL4_Word field, seL4_Word value);
void vmm_vmcs_init_guest(vmm_vcpu_t *vcpu);

#endif /* _LIB_VMM_VMCS_H_ */

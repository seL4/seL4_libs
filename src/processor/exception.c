/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/*vm exit handler for exceptions*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

#include <sel4/sel4.h>

#include "vmm/config.h"
#include "vmm/vmcs.h"

#include "vmm/vmm.h"

#include "vmm/vmexit.h"

#if 0    

static inline bool is_pagefault(unsigned int intr_info) {
	return (intr_info & (INTR_INFO_INTR_TYPE_MASK | INTR_INFO_VECTOR_MASK |
			     INTR_INFO_VALID_MASK)) ==
		(INTR_TYPE_HARD_EXCEPTION | PF_VECTOR | INTR_INFO_VALID_MASK);
}

static inline bool is_no_device(unsigned int intr_info) {
	return (intr_info & (INTR_INFO_INTR_TYPE_MASK | INTR_INFO_VECTOR_MASK |
			     INTR_INFO_VALID_MASK)) ==
		(INTR_TYPE_HARD_EXCEPTION | NM_VECTOR | INTR_INFO_VALID_MASK);
}

static inline bool is_invalid_opcode(unsigned int intr_info) {
	return (intr_info & (INTR_INFO_INTR_TYPE_MASK | INTR_INFO_VECTOR_MASK |
			     INTR_INFO_VALID_MASK)) ==
		(INTR_TYPE_HARD_EXCEPTION | UD_VECTOR | INTR_INFO_VALID_MASK);
}

static inline bool is_external_interrupt(unsigned int intr_info) { 
	return (intr_info & (INTR_INFO_INTR_TYPE_MASK | INTR_INFO_VALID_MASK))
		== (INTR_TYPE_EXT_INTR | INTR_INFO_VALID_MASK);
}

static inline bool is_machine_check(unsigned int intr_info) {
	return (intr_info & (INTR_INFO_INTR_TYPE_MASK | INTR_INFO_VECTOR_MASK |
			     INTR_INFO_VALID_MASK)) ==
		(INTR_TYPE_HARD_EXCEPTION | MC_VECTOR | INTR_INFO_VALID_MASK);
}


static int vmm_exception_handle_machine_check(void) {

    return LIB_VMM_ERR;
}


static int vmm_exception_handle_nmi(void) {

/*FIXME: HANDLE NMI FAULT*/
    return LIB_VMM_ERR;
}


static int vmm_exception_handle_no_device(void) {

    return LIB_VMM_ERR;
}

static int vmm_exception_handle_invalid_opcode(void) {

    return LIB_VMM_ERR;
}


static int vmm_exception_handle_pagefault(void) {

    return LIB_VMM_ERR;
}
#endif


/*handler for exception or NMI*/
int vmm_exception_handler(gcb_t *guest) {
    uint32_t interupt = vmm_vmcs_read(LIB_VMM_VCPU_CAP, VMX_DATA_EXIT_INTERRUPT_INFO);
    uint32_t extra = vmm_vmcs_read(LIB_VMM_VCPU_CAP, VMX_DATA_EXIT_INTERRUPT_ERROR);
    printf("vmm exception interrupt information 0x%x error 0x%x\n", interupt, extra);
//    assert(!"Not implemented");
    return LIB_VMM_ERR;

#if 0    
    int ret = LIB_VMM_ERR;

    unsigned int error_code;

    unsigned int intr_info = vmm_vmcs_read(LIB_VMM_VCPU_CAP, VMX_DATA_EXIT_INTERRUPT_INFO);

    dprintf(4, "vmm exception interrupt info value 0x%x\n", intr_info);

    if (is_machine_check(intr_info)) 
        ret = vmm_exception_handle_machine_check();

    if (is_no_device(intr_info))
        ret = vmm_exception_handle_no_device();

    if (is_invalid_opcode(intr_info)) 
        ret = vmm_exception_handle_invalid_opcode();


    if ((intr_info & INTR_INFO_INTR_TYPE_MASK) == INTR_TYPE_NMI_INTR &&
            (intr_info & INTR_INFO_VALID_MASK))  
        ret = vmm_exception_handle_nmi();

    /*exceptions with error code*/
    if (intr_info & INTR_INFO_DELIVER_CODE_MASK)
        error_code = vmm_vmcs_read(LIB_VMM_VCPU_CAP, VMX_DATA_EXIT_INTERRUPT_ERROR);

    if (is_pagefault(intr_info))
        ret = vmm_exception_handle_pagefault();


    return ret;
#endif
}



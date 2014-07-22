/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/*This file contains structure definitions and function prototype related with VMM.*/

#ifndef _LIB_VMM_VMM_H_
#define _LIB_VMM_VMM_H_

#include <autoconf.h>
#include <sel4/sel4.h>

#include "vmm/processor/platfeature.h"


#define LIB_VMM_VM_EXIT_MSG_LEN    21


/*the init value for vmcs*/
#define VMM_VMCS_CR0_MASK           (X86_CR0_PG | X86_CR0_PE)
#define VMM_VMCS_CR0_SHADOW         (X86_CR0_PG | X86_CR0_PE)

#ifdef CONFIG_VMM_ENABLE_PAE
#define VMM_VMCS_CR4_MASK           (X86_CR4_PSE | X86_CR4_PAE)
#else
#define VMM_VMCS_CR4_MASK           (X86_CR4_PSE)
#endif

#define VMM_VMCS_CR4_SHADOW         VMM_VMCS_CR4_MASK




/*GPR sequence, strictly follow the qualification rules (vm exits)*/
enum kvm_reg {
	VCPU_REGS_RAX = 0,
	VCPU_REGS_RCX = 1,
	VCPU_REGS_RDX = 2,
	VCPU_REGS_RBX = 3,
	VCPU_REGS_RSP = 4,
	VCPU_REGS_RBP = 5,
	VCPU_REGS_RSI = 6,
	VCPU_REGS_RDI = 7,
	VCPU_REGS_RIP,
	NR_VCPU_REGS
};



/*guest control block*/
typedef struct gcb {
    
    seL4_UserContext context;

    /*exit information */
    unsigned int reason;
    unsigned int qualification;
    unsigned int instruction_length;
    unsigned int guest_physical;
    unsigned int rflags;
    unsigned int guest_interruptibility;
    unsigned int control_entry;
    unsigned int sender;

    /*are we hlt'ed waiting for an interrupted */
    int interrupt_halt;

    /*vmcs setting*/
    unsigned int cr0;
    unsigned int cr0_mask;
    unsigned int cr0_shadow;
    unsigned int cr2;
    unsigned int cr3;
    unsigned int cr4;
    unsigned int cr4_mask;
    unsigned int cr4_shadow;

    /*replicated reg set used by vm exit handlers*/
    unsigned int regs[NR_VCPU_REGS];
    
    unsigned int state;
} gcb_t;





/*debugging interfaces*/
void vmm_print_guest_context(int);
void vmm_print_ipc_msg(seL4_Word);


/*interfaces in the vmm moudle*/
int vmm_vmcs_read(seL4_CPtr vcpu, unsigned int field);
void vmm_vmcs_write(seL4_CPtr vcpu, unsigned int field, unsigned int value); 
void vmm_vmcs_init_guest(seL4_CPtr vcpu, seL4_Word pd);


/*external interfaces*/
/*init vmm moudle */
void vmm_init(void);

/*running vmm moudle*/
void vmm_run(void);

void vmm_have_pending_interrupt(gcb_t *guest);
int interrupt_pending(gcb_t *guest);

#endif

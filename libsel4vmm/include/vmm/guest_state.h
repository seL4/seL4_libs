/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#pragma once

#include <stdlib.h>

#include <sel4/sel4.h>

#include "vmm/vmcs.h"
#include "vmm/platform/vmcs.h"

typedef struct guest_cr_virt_state {
    /* mask represents bits that are owned by us, the host */
    unsigned int cr0_mask;
    unsigned int cr4_mask;
    /* the shadow represents what the values of our owned bits should be seen as by the guest.
     * i.e. the value they set it to */
    unsigned int cr0_shadow;
    unsigned int cr4_shadow;
    /* for any bits owned by us, this represents what those bits should actually be set to */
    unsigned int cr0_host_bits;
    unsigned int cr4_host_bits;
    /* the raw cr3 is only valid if we're trapping guest accesses to cr3, which we
     * only do if the guest has not yet enabled paging for itself. If the guest has
     * enabled paging then the value should be retrieved from the guest machine state */
    uint32_t cr3_guest;
} guest_cr_virt_state_t;

typedef struct  guest_exit_information {
    bool in_exit;
    unsigned int reason;
    unsigned int qualification;
    unsigned int instruction_length;
    unsigned int guest_physical;
} guest_exit_information_t;

typedef enum machine_state_status {
    /* Unknown means guest has run and we have no idea if
     * our value is up to date */
    machine_state_unknown = 0,
    /* Valid means the value we have is up to date and we
     * have not modified it with respect to what the guest
     * believes */
    machine_state_valid,
    /* As part of the virtualization we have modified the value
     * and this needs to be sycned back to the guest */
    machine_state_modified
} machine_state_status_t;

#define MACHINE_STATE(type, name) machine_state_status_t name##_status; type name
#define MACHINE_STATE_READ(name, value) do { assert(name##_status == machine_state_unknown); name##_status = machine_state_valid; name = (value);} while(0)
#define MACHINE_STATE_DIRTY(name) do { name##_status = machine_state_modified; } while(0)
#define MACHINE_STATE_SYNC(name) do { assert(name##_status == machine_state_modified); name##_status = machine_state_valid; } while(0)
#define MACHINE_STATE_INVAL(name) do { assert(name##_status != machine_state_modified); name##_status = machine_state_unknown; } while(0)
#define MACHINE_STATE_SYNC_INVAL(name) do { MACHINE_STATE_SYNC(name); MACHINE_STATE_INVAL(name); } while(0)
#define IS_MACHINE_STATE_VALID(name) (name##_status == machine_state_valid)
#define IS_MACHINE_STATE_UNKNOWN(name) (name##_status == machine_state_unknown)
#define IS_MACHINE_STATE_MODIFIED(name) (name##_status == machine_state_modified)

typedef struct guest_machine_state {
    MACHINE_STATE(seL4_VCPUContext, context);
    MACHINE_STATE(unsigned int, cr0);
    MACHINE_STATE(unsigned int, cr3);
    MACHINE_STATE(unsigned int, cr4);
    MACHINE_STATE(unsigned int, rflags);
    MACHINE_STATE(unsigned int, guest_interruptibility);
    MACHINE_STATE(unsigned int, idt_base);
    MACHINE_STATE(unsigned int, idt_limit);
    MACHINE_STATE(unsigned int, gdt_base);
    MACHINE_STATE(unsigned int, gdt_limit);
    MACHINE_STATE(unsigned int, cs_selector);
    MACHINE_STATE(unsigned int, entry_exception_error_code);
    /* This is state that we set on VMentry and get back on
     * a vmexit, therefore it is always valid and correct */
    unsigned int eip;
    unsigned int control_entry;
    unsigned int control_ppc;
} guest_machine_state_t;

/* Define the seL4_UserContext layout so we can treat it as an array */
#define USER_CONTEXT_EAX 0
#define USER_CONTEXT_EBX 1
#define USER_CONTEXT_ECX 2
#define USER_CONTEXT_EDX 3
#define USER_CONTEXT_ESI 4
#define USER_CONTEXT_EDI 5
#define USER_CONTEXT_EBP 6

typedef struct guest_virt_state {
    guest_cr_virt_state_t cr;
    /* are we hlt'ed waiting for an interrupted */
    int interrupt_halt;
} guest_virt_state_t;

typedef struct guest_state {
    /* Guest machine state. Bits of this may or may not be valid.
     * This is all state that the guest itself observes and modifies.
     * Either explicitly (registers) or implicitly (interruptability state) */
    guest_machine_state_t machine;
    /* Internal state that we use for handling guest state virtualization. */
    guest_virt_state_t virt;
    /* Information relating specifically to a guest exist, that is generated
     * as a result of the exit */
    guest_exit_information_t exit;
} guest_state_t;

static inline bool vmm_guest_state_no_modified(guest_state_t *gs) {
    return !(
        IS_MACHINE_STATE_MODIFIED(gs->machine.context) ||
        IS_MACHINE_STATE_MODIFIED(gs->machine.cr0) ||
        IS_MACHINE_STATE_MODIFIED(gs->machine.cr3) ||
        IS_MACHINE_STATE_MODIFIED(gs->machine.cr4) ||
        IS_MACHINE_STATE_MODIFIED(gs->machine.rflags) ||
        IS_MACHINE_STATE_MODIFIED(gs->machine.guest_interruptibility) ||
        IS_MACHINE_STATE_MODIFIED(gs->machine.idt_base) ||
        IS_MACHINE_STATE_MODIFIED(gs->machine.idt_limit) ||
        IS_MACHINE_STATE_MODIFIED(gs->machine.gdt_base) ||
        IS_MACHINE_STATE_MODIFIED(gs->machine.gdt_limit) ||
        IS_MACHINE_STATE_MODIFIED(gs->machine.cs_selector) ||
        IS_MACHINE_STATE_MODIFIED(gs->machine.entry_exception_error_code)
    );
}

static inline void vmm_guest_state_invalidate_all(guest_state_t *gs) {
    MACHINE_STATE_INVAL(gs->machine.context);
    MACHINE_STATE_INVAL(gs->machine.cr0);
    MACHINE_STATE_INVAL(gs->machine.cr3);
    MACHINE_STATE_INVAL(gs->machine.cr4);
    MACHINE_STATE_INVAL(gs->machine.rflags);
    MACHINE_STATE_INVAL(gs->machine.guest_interruptibility);
    MACHINE_STATE_INVAL(gs->machine.idt_base);
    MACHINE_STATE_INVAL(gs->machine.idt_limit);
    MACHINE_STATE_INVAL(gs->machine.gdt_base);
    MACHINE_STATE_INVAL(gs->machine.gdt_limit);
    MACHINE_STATE_INVAL(gs->machine.cs_selector);
    MACHINE_STATE_INVAL(gs->machine.entry_exception_error_code);
}

/* get */
static inline uint32_t vmm_read_user_context(guest_state_t *gs, int reg) {
    assert(!IS_MACHINE_STATE_UNKNOWN(gs->machine.context));
    assert(reg >= 0 && reg <= USER_CONTEXT_EBP);
    return (&gs->machine.context.eax)[reg];
}

static inline unsigned int vmm_guest_state_get_eip(guest_state_t *gs) {
    return gs->machine.eip;
}

static inline unsigned int vmm_guest_state_get_cr0(guest_state_t *gs, seL4_CPtr vcpu) {
    if (IS_MACHINE_STATE_UNKNOWN(gs->machine.cr0)) {
        MACHINE_STATE_READ(gs->machine.cr0, vmm_vmcs_read(vcpu, VMX_GUEST_CR0));
    }
    return gs->machine.cr0;
}

static inline unsigned int vmm_guest_state_get_cr3(guest_state_t *gs, seL4_CPtr vcpu) {
    if (IS_MACHINE_STATE_UNKNOWN(gs->machine.cr3)) {
        MACHINE_STATE_READ(gs->machine.cr3, vmm_vmcs_read(vcpu, VMX_GUEST_CR3));
    }
    return gs->machine.cr3;
}

static inline unsigned int vmm_guest_state_get_cr4(guest_state_t *gs, seL4_CPtr vcpu) {
    if (IS_MACHINE_STATE_UNKNOWN(gs->machine.cr4)) {
        MACHINE_STATE_READ(gs->machine.cr4, vmm_vmcs_read(vcpu, VMX_GUEST_CR4));
    }
    return gs->machine.cr4;
}

static inline unsigned int vmm_guest_state_get_rflags(guest_state_t *gs, seL4_CPtr vcpu) {
    if (IS_MACHINE_STATE_UNKNOWN(gs->machine.rflags)) {
        MACHINE_STATE_READ(gs->machine.rflags, vmm_vmcs_read(vcpu, VMX_GUEST_RFLAGS));
    }
    return gs->machine.rflags;
}

static inline unsigned int vmm_guest_state_get_interruptibility(guest_state_t *gs, seL4_CPtr vcpu) {
    if (IS_MACHINE_STATE_UNKNOWN(gs->machine.guest_interruptibility)) {
        MACHINE_STATE_READ(gs->machine.guest_interruptibility, vmm_vmcs_read(vcpu, VMX_GUEST_INTERRUPTABILITY));
    }
    return gs->machine.guest_interruptibility;
}

static inline unsigned int vmm_guest_state_get_control_entry(guest_state_t *gs) {
    return gs->machine.control_entry;
}

static inline unsigned int vmm_guest_state_get_control_ppc(guest_state_t *gs) {
    return gs->machine.control_ppc;
}

static inline unsigned int vmm_guest_state_get_idt_base(guest_state_t *gs, seL4_CPtr vcpu) {
    if (IS_MACHINE_STATE_UNKNOWN(gs->machine.idt_base)) {
        MACHINE_STATE_READ(gs->machine.idt_base, vmm_vmcs_read(vcpu, VMX_GUEST_IDTR_BASE));
    }
    return gs->machine.idt_base;
}

static inline unsigned int vmm_guest_state_get_idt_limit(guest_state_t *gs, seL4_CPtr vcpu) {
    if (IS_MACHINE_STATE_UNKNOWN(gs->machine.idt_limit)) {
        MACHINE_STATE_READ(gs->machine.idt_limit, vmm_vmcs_read(vcpu, VMX_GUEST_IDTR_LIMIT));
    }
    return gs->machine.idt_limit;
}

static inline unsigned int vmm_guest_state_get_gdt_base(guest_state_t *gs, seL4_CPtr vcpu) {
    if (IS_MACHINE_STATE_UNKNOWN(gs->machine.gdt_base)) {
        MACHINE_STATE_READ(gs->machine.gdt_base, vmm_vmcs_read(vcpu, VMX_GUEST_GDTR_BASE));
    }
    return gs->machine.gdt_base;
}

static inline unsigned int vmm_guest_state_get_gdt_limit(guest_state_t *gs, seL4_CPtr vcpu) {
    if (IS_MACHINE_STATE_UNKNOWN(gs->machine.gdt_limit)) {
        MACHINE_STATE_READ(gs->machine.gdt_limit, vmm_vmcs_read(vcpu, VMX_GUEST_GDTR_LIMIT));
    }
    return gs->machine.gdt_limit;
}

static inline unsigned int vmm_guest_state_get_cs_selector(guest_state_t *gs, seL4_CPtr vcpu) {
    if (IS_MACHINE_STATE_UNKNOWN(gs->machine.cs_selector)) {
        MACHINE_STATE_READ(gs->machine.cs_selector, vmm_vmcs_read(vcpu, VMX_GUEST_CS_SELECTOR));
    }
    return gs->machine.cs_selector;
}

/* set */
static inline void vmm_set_user_context(guest_state_t *gs, int reg, uint32_t val) {
    MACHINE_STATE_DIRTY(gs->machine.context);
    assert(reg >= 0 && reg <= USER_CONTEXT_EBP);
    (&gs->machine.context.eax)[reg] = val;
}

static inline void vmm_guest_state_set_eip(guest_state_t *gs, unsigned int val) {
    gs->machine.eip = val;
}

static inline void vmm_guest_state_set_cr0(guest_state_t *gs, unsigned int val) {
    MACHINE_STATE_DIRTY(gs->machine.cr0);
    gs->machine.cr0 = val;
}

static inline void vmm_guest_state_set_cr3(guest_state_t *gs, unsigned int val) {
    MACHINE_STATE_DIRTY(gs->machine.cr3);
    gs->machine.cr3 = val;
}

static inline void vmm_guest_state_set_cr4(guest_state_t *gs, unsigned int val) {
    MACHINE_STATE_DIRTY(gs->machine.cr4);
    gs->machine.cr4 = val;
}

static inline void vmm_guest_state_set_control_entry(guest_state_t *gs, unsigned int val) {
    gs->machine.control_entry = val;
}

static inline void vmm_guest_state_set_control_ppc(guest_state_t *gs, unsigned int val) {
    gs->machine.control_ppc = val;
}

static inline void vmm_guest_state_set_idt_base(guest_state_t *gs, unsigned int val) {
    MACHINE_STATE_DIRTY(gs->machine.idt_base);
    gs->machine.idt_base = val;
}

static inline void vmm_guest_state_set_idt_limit(guest_state_t *gs, unsigned int val) {
    MACHINE_STATE_DIRTY(gs->machine.idt_limit);
    gs->machine.idt_limit = val;
}

static inline void vmm_guest_state_set_gdt_base(guest_state_t *gs, unsigned int val) {
    MACHINE_STATE_DIRTY(gs->machine.gdt_base);
    gs->machine.gdt_base = val;
}

static inline void vmm_guest_state_set_gdt_limit(guest_state_t *gs, unsigned int val) {
    MACHINE_STATE_DIRTY(gs->machine.gdt_limit);
    gs->machine.gdt_limit = val;
}

static inline void vmm_guest_state_set_cs_selector(guest_state_t *gs, unsigned int val) {
    MACHINE_STATE_DIRTY(gs->machine.cs_selector);
    gs->machine.cs_selector = val;
}

static inline void vmm_guest_state_set_entry_exception_error_code(guest_state_t *gs, unsigned int val) {
    MACHINE_STATE_DIRTY(gs->machine.entry_exception_error_code);
    gs->machine.entry_exception_error_code = val;
}

/* sync */
static inline void vmm_guest_state_sync_cr0(guest_state_t *gs, seL4_CPtr vcpu) {
    if(IS_MACHINE_STATE_MODIFIED(gs->machine.cr0)) {
        vmm_vmcs_write(vcpu, VMX_GUEST_CR0, gs->machine.cr0);
        MACHINE_STATE_SYNC(gs->machine.cr0);
    }
}

static inline void vmm_guest_state_sync_cr3(guest_state_t *gs, seL4_CPtr vcpu) {
    if(IS_MACHINE_STATE_MODIFIED(gs->machine.cr3)) {
        vmm_vmcs_write(vcpu, VMX_GUEST_CR3, gs->machine.cr3);
        MACHINE_STATE_SYNC(gs->machine.cr3);
    }
}

static inline void vmm_guest_state_sync_cr4(guest_state_t *gs, seL4_CPtr vcpu) {
    if(IS_MACHINE_STATE_MODIFIED(gs->machine.cr4)) {
        vmm_vmcs_write(vcpu, VMX_GUEST_CR4, gs->machine.cr4);
        MACHINE_STATE_SYNC(gs->machine.cr4);
    }
}

static inline void vmm_guest_state_sync_idt_base(guest_state_t *gs, seL4_CPtr vcpu) {
    if(IS_MACHINE_STATE_MODIFIED(gs->machine.idt_base)) {
        vmm_vmcs_write(vcpu, VMX_GUEST_IDTR_BASE, gs->machine.idt_base);
        MACHINE_STATE_SYNC(gs->machine.idt_base);
    }
}

static inline void vmm_guest_state_sync_idt_limit(guest_state_t *gs, seL4_CPtr vcpu) {
    if(IS_MACHINE_STATE_MODIFIED(gs->machine.idt_limit)) {
        vmm_vmcs_write(vcpu, VMX_GUEST_IDTR_LIMIT, gs->machine.idt_limit);
        MACHINE_STATE_SYNC(gs->machine.idt_limit);
    }
}

static inline void vmm_guest_state_sync_gdt_base(guest_state_t *gs, seL4_CPtr vcpu) {
    if(IS_MACHINE_STATE_MODIFIED(gs->machine.gdt_base)) {
        vmm_vmcs_write(vcpu, VMX_GUEST_GDTR_BASE, gs->machine.gdt_base);
        MACHINE_STATE_SYNC(gs->machine.gdt_base);
    }
}

static inline void vmm_guest_state_sync_gdt_limit(guest_state_t *gs, seL4_CPtr vcpu) {
    if(IS_MACHINE_STATE_MODIFIED(gs->machine.gdt_limit)) {
        vmm_vmcs_write(vcpu, VMX_GUEST_GDTR_LIMIT, gs->machine.gdt_limit);
        MACHINE_STATE_SYNC(gs->machine.gdt_limit);
    }
}

static inline void vmm_guest_state_sync_cs_selector(guest_state_t *gs, seL4_CPtr vcpu) {
    if(IS_MACHINE_STATE_MODIFIED(gs->machine.cs_selector)) {
        vmm_vmcs_write(vcpu, VMX_GUEST_CS_SELECTOR, gs->machine.cs_selector);
        MACHINE_STATE_SYNC(gs->machine.cs_selector);
    }
}

static inline void vmm_guest_state_sync_entry_exception_error_code(guest_state_t *gs, seL4_CPtr vcpu) {
    if (IS_MACHINE_STATE_MODIFIED(gs->machine.entry_exception_error_code)) {
        vmm_vmcs_write(vcpu, VMX_CONTROL_ENTRY_EXCEPTION_ERROR_CODE, gs->machine.entry_exception_error_code);
        MACHINE_STATE_SYNC(gs->machine.entry_exception_error_code);
    }
}

/* Exit */
static inline unsigned int vmm_guest_exit_get_reason(guest_state_t *gs) {
    assert(gs->exit.in_exit);
    return gs->exit.reason;
}

static inline unsigned int vmm_guest_exit_get_physical(guest_state_t *gs) {
    assert(gs->exit.in_exit);
    return gs->exit.guest_physical;
}

static inline unsigned int vmm_guest_exit_get_int_len(guest_state_t *gs) {
    assert(gs->exit.in_exit);
    return gs->exit.instruction_length;
}

static inline unsigned int vmm_guest_exit_get_qualification(guest_state_t *gs) {
    assert(gs->exit.in_exit);
    return gs->exit.qualification;
}

static inline void vmm_guest_exit_next_instruction(guest_state_t *gs, seL4_CPtr vcpu) {
    vmm_guest_state_set_eip(gs, vmm_guest_state_get_eip(gs) + vmm_guest_exit_get_int_len(gs));
}


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

#include "vmm/vmm.h"
#include "vmm/guest_state.h"

int vmm_fetch_instruction(vmm_vcpu_t *vcpu, uint32_t eip, uintptr_t cr3, int len, uint8_t *buf);

int vmm_decode_instruction(uint8_t *instr, int instr_len, int *reg, uint32_t *imm, int *op_len);

/* Interpret just enough virtual 8086 instructions to run trampoline code.
   Returns the final jump address */
uintptr_t vmm_emulate_realmode(guest_memory_t *gm, uint8_t *instr_buf,
        uint16_t *segment, uintptr_t eip, uint32_t len, guest_state_t *gs);

// TODO don't have these in a header, make them inline functions
const static int vmm_decoder_reg_mapw[] = {
    USER_CONTEXT_EAX,
    USER_CONTEXT_ECX,
    USER_CONTEXT_EDX,
    USER_CONTEXT_EBX,
    /*USER_CONTEXT_ESP*/-1,
    USER_CONTEXT_EBP,
    USER_CONTEXT_ESI,
    USER_CONTEXT_EDI
};

const static int vmm_decoder_reg_mapb[] = {
    USER_CONTEXT_EAX,
    USER_CONTEXT_ECX,
    USER_CONTEXT_EDX,
    USER_CONTEXT_EBX,
    USER_CONTEXT_EAX,
    USER_CONTEXT_ECX,
    USER_CONTEXT_EDX,
    USER_CONTEXT_EBX
};


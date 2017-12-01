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

/* x86 fetch/decode/emulate code

Author: W.A.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vmm/debug.h"
#include "vmm/platform/guest_vspace.h"
#include "vmm/platform/guest_memory.h"
#include "vmm/guest_state.h"
#include "vmm/processor/platfeature.h"

/* TODO are these defined elsewhere? */
#define IA32_PDE_SIZE(pde) (pde & BIT(7))
#define IA32_PDE_PRESENT(pde) (pde & BIT(0))
#define IA32_PTE_ADDR(pte) (pte & 0xFFFFF000)
#define IA32_PSE_ADDR(pde) (pde & 0xFFC00000)

#define IA32_OPCODE_S(op) (op & BIT(0))
#define IA32_OPCODE_D(op) (op & BIT(1))
#define IA32_OPCODY_BODY(op) (op & 0b11111100)
#define IA32_MODRM_REG(m) ((m & 0b00111000) >> 3)

#define SEG_MULT (0x10)

/* Get a word from a guest physical address */
inline static uint32_t guest_get_phys_word(vmm_t *vmm, uintptr_t addr) {
    uint32_t val;

    vmm_guest_vspace_touch(&vmm->guest_mem.vspace, addr, sizeof(uint32_t),
            vmm_guest_get_phys_data_help, &val);

    return val;
}

/* Fetch a guest's instruction */
int vmm_fetch_instruction(vmm_vcpu_t *vcpu, uint32_t eip, uintptr_t cr3,
        int len, uint8_t *buf) {
    /* Walk page tables to get physical address of instruction */
    uintptr_t instr_phys = 0;

    /* ensure that PAE is not enabled */
    if (vmm_guest_state_get_cr4(&vcpu->guest_state, vcpu->guest_vcpu) & X86_CR4_PAE) {
        ZF_LOGE("Do not support walking PAE paging structures");
        return -1;
    }

    // TODO implement page-boundary crossing properly
    assert((eip >> 12) == ((eip + len) >> 12));

    uint32_t pdi = eip >> 22;
    uint32_t pti = (eip >> 12) & 0x3FF;

    uint32_t pde = guest_get_phys_word(vcpu->vmm, cr3 + pdi * 4);

    assert(IA32_PDE_PRESENT(pde)); /* WTF? */

    if (IA32_PDE_SIZE(pde)) {
        /* PSE is used, 4M pages */
        instr_phys = (uintptr_t)IA32_PSE_ADDR(pde) + (eip & 0x3FFFFF);
    } else {
        /* 4k pages */
        uint32_t pte = guest_get_phys_word(vcpu->vmm,
                (uintptr_t)IA32_PTE_ADDR(pde) + pti * 4);

        assert(IA32_PDE_PRESENT(pte));

        instr_phys = (uintptr_t)IA32_PTE_ADDR(pte) + (eip & 0xFFF);
    }

    /* Fetch instruction */
    vmm_guest_vspace_touch(&vcpu->vmm->guest_mem.vspace, instr_phys, len,
            vmm_guest_get_phys_data_help, buf);

    return 0;
}

/* Returns 1 if this byte is an x86 instruction prefix */
static int is_prefix(uint8_t byte) {
    switch (byte) {
        case 0x26:
        case 0x2e:
        case 0x36:
        case 0x3e:
        case 0x64:
        case 0x65:
        case 0x67:
        case 0x66:
            return 1;
    }

    return 0;
}

static void debug_print_instruction(uint8_t *instr, int instr_len) {
    printf("instruction dump: ");
    for (int j = 0; j < instr_len; j++) {
        printf("%2x ", instr[j]);
    }
    printf("\n");
}

/* Partial support to decode an instruction for a memory access
   This is very crude. It can break in many ways. */
int vmm_decode_instruction(uint8_t *instr, int instr_len, int *reg, uint32_t *imm, int *op_len) {
    /* First loop through and check prefixes */
    int oplen = 1; /* Operand length */
    int i;
    for (i = 0; i < instr_len; i++) {
        if (is_prefix(instr[i])) {
            if (instr[i] == 0x66) {
                /* 16 bit modifier */
                oplen = 2;
            }
        } else {
            /* We've hit the opcode */
            break;
        }
    }
    assert(i < instr_len); /* We still need an opcode */

    uint8_t opcode = instr[i];
    //uint8_t opcode_ex = 0;
    if (opcode == 0x0f) {
        printf("can't emulate instruction with multi-byte opcode!\n");
        debug_print_instruction(instr, instr_len);
        assert(0); /* We don't handle >1 byte opcodes */
    }
    if (oplen != 2 && IA32_OPCODE_S(opcode)) {
        oplen = 4;
    }

    uint8_t modrm = instr[++i];
    switch (opcode) {
        case 0x88:
        case 0x89:
        case 0x8a:
        case 0x8b:
        case 0x8c:
            // Mov with register
            *reg = IA32_MODRM_REG(modrm);
            *op_len = oplen;
            break;
        case 0xc6:
        case 0xc7:
            // Mov with immediate
            *reg = -1;
            *op_len = oplen;
            uint32_t immediate = 0;
            for (int j = 0; j < oplen; j++) {
                immediate <<= 8;
                immediate |= instr[instr_len - j - 1];
            }
            *imm = immediate;
            break;
        default:
            printf("can't emulate this instruction!\n");
            debug_print_instruction(instr, instr_len);
            assert(0);
    }

    return 0;
}

/*
   Useful information: The GDT loaded by the Linux SMP trampoline looks like:
0x00: 00 00 00 00 00 00 00 00
0x08: 00 00 00 00 00 00 00 00
0x10: ff ff 00 00 00 9b cf 00 <- Executable 0x00000000-0xffffffff
0x18: ff ff 00 00 00 93 cf 00 <- RW data    0x00000000-0xffffffff
*/

/* Interpret just enough virtual 8086 instructions to run trampoline code.
   Returns the final jump address */
uintptr_t vmm_emulate_realmode(guest_memory_t *gm, uint8_t *instr_buf,
        uint16_t *segment, uintptr_t eip, uint32_t len, guest_state_t *gs)
{
    /* We only track one segment, and assume that code and data are in the same
       segment, which is valid for most trampoline and bootloader code */
    uint8_t *instr = instr_buf;
    assert(segment);

    while (instr - instr_buf < len) {
        uintptr_t mem = 0;
        uint32_t lit = 0;
        int m66 = 0;

        uint32_t base = 0;
        uint32_t limit = 0;

        if (*instr == 0x66) {
            m66 = 1;
            instr++;
        }

        if (*instr == 0x0f) {
            instr++;
            if (*instr == 0x01) {
                instr++;
                if (*instr == 0x1e) {
                    // lidtl
                    instr++;
                    memcpy(&mem, instr, 2);
                    mem += *segment * SEG_MULT;
                    instr += 2;

                    /* Limit is first 2 bytes, base is next 4 bytes */
                    vmm_guest_vspace_touch(&gm->vspace, mem,
                            2, vmm_guest_get_phys_data_help, &limit);
                    vmm_guest_vspace_touch(&gm->vspace, mem + 2,
                            4, vmm_guest_get_phys_data_help, &base);
                    DPRINTF(4, "lidtl %p\n", (void*)mem);

                    vmm_guest_state_set_idt_base(gs, base);
                    vmm_guest_state_set_idt_limit(gs, limit);
                } else if (*instr == 0x16) {
                    // lgdtl
                    instr++;
                    memcpy(&mem, instr, 2);
                    mem += *segment * SEG_MULT;
                    instr += 2;

                    /* Limit is first 2 bytes, base is next 4 bytes */
                    vmm_guest_vspace_touch(&gm->vspace, mem,
                            2, vmm_guest_get_phys_data_help, &limit);
                    vmm_guest_vspace_touch(&gm->vspace, mem + 2,
                            4, vmm_guest_get_phys_data_help, &base);
                    DPRINTF(4, "lgdtl %p; base = %x, limit = %x\n", (void*)mem,
                            base, limit);

                    vmm_guest_state_set_gdt_base(gs, base);
                    vmm_guest_state_set_gdt_limit(gs, limit);
                } else {
                    //ignore
                    instr++;
                }
            } else {
                //ignore
                instr++;
            }
        } else if (*instr == 0xea) {
            /* Absolute jmp */
            instr++;
            uint32_t base = 0;
            uintptr_t jmp_addr = 0;
            if (m66) {
                // base is 4 bytes
                /* Make the wild assumptions that we are now in protected mode
                   and the relevant GDT entry just covers all memory. Therefore
                   the base address is our absolute address. This just happens
                   to work with Linux and probably other modern systems that
                   don't use the GDT much. */
                memcpy(&base, instr, 4);
                instr += 4;
                jmp_addr = base;
                memcpy(segment, instr, 2);
            } else {
                memcpy(&base, instr, 2);
                instr += 2;
                memcpy(segment, instr, 2);
                jmp_addr = *segment * SEG_MULT + base;
            }
            instr += 2;
            DPRINTF(4, "absolute jmpf $%p, cs now %04x\n", (void*)jmp_addr, *segment);
            if (((int64_t)jmp_addr - (int64_t)(len + eip)) >= 0) {
                vmm_guest_state_set_cs_selector(gs, *segment);
                return jmp_addr;
            } else {
                instr = jmp_addr - eip + instr_buf;
            }
        } else {
            switch (*instr) {
                case 0xa1:
                    /* mov offset memory to eax */
                    instr++;
                    memcpy(&mem, instr, 2);
                    instr += 2;
                    mem += *segment * SEG_MULT;
                    DPRINTF(4, "mov %p, eax\n", (void*)mem);
                    uint32_t eax;
                    vmm_guest_vspace_touch(&gm->vspace, mem,
                            4, vmm_guest_get_phys_data_help, &eax);
                    vmm_set_user_context(gs, USER_CONTEXT_EAX, eax);
                    break;
                case 0xc7:
                    instr++;
                    if (*instr == 0x06) { // modrm
                        int size;
                        instr++;
                        /* mov literal to memory */
                        memcpy(&mem, instr, 2);
                        mem += *segment * SEG_MULT;
                        instr += 2;
                        if (m66) {
                            memcpy(&lit, instr, 4);
                            size = 4;
                        } else {
                            memcpy(&lit, instr, 2);
                            size = 2;
                        }
                        instr += size;
                        DPRINTF(4, "mov $0x%x, %p\n", lit, (void*)mem);
                        vmm_guest_vspace_touch(&gm->vspace, mem,
                                size, vmm_guest_set_phys_data_help, &lit);
                    }
                    break;
                case 0xba:
                    //?????mov literal to dx
                    /* ignore */
                    instr += 2;
                    break;
                case 0x8c:
                case 0x8e:
                    /* mov to/from sreg. ignore */
                    instr += 2;
                    break;
                default:
                    /* Assume this is a single byte instruction we can ignore */
                    instr++;
            }
        }

        DPRINTF(6, "read %zu bytes\n", (size_t)(instr - instr_buf));
    }

    return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vmm/debug.h"
#include "vmm/platform/guest_vspace.h"
#include "vmm/platform/guest_memory.h"

// TODO are these defined elsewhere?
#define IA32_PDE_SIZE(pde) (pde & BIT(7))
#define IA32_PDE_PRESENT(pde) (pde & BIT(0))
#define IA32_PTE_ADDR(pte) (pte & 0xFFFFF000)
#define IA32_PSE_ADDR(pde) (pde & 0xFFC00000)


#define IA32_OPCODE_S(op) (op & BIT(0))
#define IA32_OPCODE_D(op) (op & BIT(1))
#define IA32_OPCODY_BODY(op) (op & 0b11111100)
#define IA32_MODRM_REG(m) ((m & 0b00111000) >> 3)

static int guest_get_phys_data_help(uintptr_t addr, void *vaddr, size_t size, size_t offset, void *cookie) {
	memcpy((char *)cookie, (char *)vaddr, size);

	return 0;
}

// Get a word from a guest physical address
inline static uint32_t guest_get_phys_word(vmm_t *vmm, uintptr_t addr) {
	uint32_t val;

	vmm_guest_vspace_touch(&vmm->guest_mem.vspace, addr, sizeof(uint32_t),
			guest_get_phys_data_help, &val);

	return val;
}

// Fetch a guest's instruction
int vmm_fetch_instruction(vmm_vcpu_t *vcpu, uint32_t eip, uintptr_t cr3, int len, uint8_t *buf) {
	if (len < 1 || len > 15) {
		len = 15;
	}

	// Walk page tables to get physical address of instruction
	uintptr_t instr_phys = 0;

	uint32_t pdi = eip >> 22;
	uint32_t pti = (eip >> 12) & 0x3FF;

	uint32_t pde = guest_get_phys_word(vcpu->vmm, cr3 + pdi * 4);

	assert(IA32_PDE_PRESENT(pde)); // WTF?

	if (IA32_PDE_SIZE(pde)) {
		// PSE is used, 4M pages
		instr_phys = (uintptr_t)IA32_PSE_ADDR(pde) + (eip & 0x3FFFFF);
	} else {
		// 4k pages
		uint32_t pte = guest_get_phys_word(vcpu->vmm,
				(uintptr_t)IA32_PTE_ADDR(pde) + pti * 4);

		assert(IA32_PDE_PRESENT(pte));

		instr_phys = (uintptr_t)IA32_PTE_ADDR(pte) + (eip & 0xFFF);
	}

	// Fetch instruction
	vmm_guest_vspace_touch(&vcpu->vmm->guest_mem.vspace, instr_phys, len,
			guest_get_phys_data_help, buf);

	return 0;
}

// Returns 1 if this byte is an x86 instruction prefix
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


// Partial support to decode an instruction for a memory access
// This is very crude. It can break in many ways.
int vmm_decode_instruction(uint8_t *instr, int instr_len, int *reg, uint32_t *imm, int *op_len) {
	// First loop through and check prefixes
	int oplen = 1; // Operand length
	int i;
	for (i = 0; i < instr_len; i++) {
		if (is_prefix(instr[i])) {
			if (instr[i] == 0x66) {
				// 16 bit modifier
				oplen = 2;
			}
		} else {
			// We've hit the opcode
			break;
		}
	}
	assert(i < instr_len); // We still need an opcode

	uint8_t opcode = instr[i];
	//uint8_t opcode_ex = 0;
	if (opcode == 0x0f) {
		printf("can't emulate instruction with multi-byte opcode!\n");
		debug_print_instruction(instr, instr_len);
		assert(0); // We don't handle >1 byte opcodes
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


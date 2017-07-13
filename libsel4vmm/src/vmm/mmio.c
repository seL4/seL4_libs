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

// Deals with ranges of memory mapped io

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sel4/sel4.h>

#include "vmm/debug.h"
#include "vmm/vmm.h"
#include "vmm/platform/vmcs.h"
#include "vmm/platform/vmexit.h"
#include "vmm/mmio.h"
#include "vmm/processor/decode.h"

#define EPT_VIOL_READ(qual) ((qual) & BIT(0))
#define EPT_VIOL_WRITE(qual) ((qual) & BIT(1))
#define EPT_VIOL_FETCH(qual) ((qual) & BIT(2))

int vmm_mmio_init(vmm_mmio_list_t *list) {
    list->num_ranges = 0;
    list->ranges = malloc(0);
    assert(list->ranges);

    return 0;
}

// Returns 0 if the exit was handled
int vmm_mmio_exit_handler(vmm_vcpu_t *vcpu, uintptr_t addr, unsigned int qualification) {
    int read = EPT_VIOL_READ(qualification);
    int write = EPT_VIOL_WRITE(qualification);
    int fetch = EPT_VIOL_FETCH(qualification);
    if (read && write) {
        // Indicates a fault while walking EPT
        return -1;
    }
    if (fetch) {
        // This is not MMIO
        return -1;
    }

    // Search the list
    for (int i = 0; i < vcpu->vmm->mmio_list.num_ranges; i++) {
        vmm_mmio_range_t *range = &vcpu->vmm->mmio_list.ranges[i];

        if (addr < range->start) {
            return -1;
        }

        if (addr <= range->end) {
            // Found a match
            if (read && range->read_handler == NULL) {
                return -1;
            }
            if (write && range->write_handler == NULL) {
                return -1;
            }

            // Decode instruction
            uint8_t ibuf[15];
            int instr_len = vmm_guest_exit_get_int_len(&vcpu->guest_state);
            vmm_fetch_instruction(vcpu,
                    vmm_guest_state_get_eip(&vcpu->guest_state),
                    vmm_guest_state_get_cr3(&vcpu->guest_state, vcpu->guest_vcpu),
                    instr_len, ibuf);

            int reg;
            uint32_t imm;
            int size;
            vmm_decode_instruction(ibuf, instr_len, &reg, &imm, &size);
assert(size == 4); // we don't support non-32 bit accesses. TODO fix this

            // Call handler
            if (read) {
                uint32_t result;
                range->read_handler(vcpu, range->cookie, addr - range->start, size, &result);

                // Inject into register
                assert(reg >= 0 && reg < 8);
                int vcpu_reg = vmm_decoder_reg_mapw[reg];
                assert(vcpu_reg >= 0);
                vmm_set_user_context(&vcpu->guest_state,
                        vcpu_reg, result);
            } else {
                // Get value to pass in
                uint32_t value = imm;
                assert (reg >= 0);
                int vcpu_reg =  vmm_decoder_reg_mapw[reg];
                assert(vcpu_reg >= 0);
                value = vmm_read_user_context(&vcpu->guest_state,
                        vcpu_reg);

                range->write_handler(vcpu, range->cookie, addr - range->start, size, value);
            }

            return 0;
        }
    }

    return -1;
}

static int range_cmp(const void *a, const void *b) {
    return ((const vmm_mmio_range_t *)a)->start - ((const vmm_mmio_range_t *)b)->start;
}

int vmm_mmio_add_handler(vmm_mmio_list_t *list, uintptr_t start, uintptr_t end,
        void *cookie, const char *name,
        vmm_mmio_read_fn read_handler, vmm_mmio_write_fn write_handler) {
    vmm_mmio_range_t *new = malloc(sizeof(*new));
    new->start = start;
    new->end = end;
    new->cookie = cookie;
    new->read_handler = read_handler;
    new->write_handler = write_handler;
    new->name = name;

    list->ranges = realloc(list->ranges, sizeof(vmm_mmio_range_t) * (list->num_ranges + 1));
    memcpy(&list->ranges[list->num_ranges++], new, sizeof(vmm_mmio_range_t));

    qsort(list->ranges, list->num_ranges, sizeof(vmm_mmio_range_t), range_cmp);

    return 0;
}

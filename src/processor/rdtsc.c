/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/* The Guest OS RDTSC instruction emulator.
 *
 *     Authors:
 *         Qian Ge
 *         Adrian Danis
 *         Xi (Ma) Chen
 */


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include <sel4/sel4.h>

#include "vmm/config.h"
#include "vmm/vmm.h"
#include "vmm/vmexit.h"

static bool _vmm_debug_rdtsc_warning = false;

typedef struct vmm_tsc_dword {
    uint32_t high;
    uint32_t low;
    uint64_t dword;
} vmm_tsc_dword_t;

/* Read the real host RDTSC. */
static inline vmm_tsc_dword_t vmm_host_read_tsc(void) {
#ifdef ARCH_IA32
    vmm_tsc_dword_t ret;
    asm volatile ("rdtsc" : "=a"(ret.low), "=d"(ret.high));
    ret.dword = (((uint64_t)ret.high) << 32) + ret.low;
    return ret;
#else
    #error "Unsupported RDTSC platform."
#endif    
}

int vmm_rdtsc_instruction_handler(gcb_t *guest) {
    // The instruction RDTSC returns the TSC in EDX:EAX.
    if (!guest) {
        return LIB_VMM_ERR;
    }

    if (!_vmm_debug_rdtsc_warning) {
        // TODO: We should emulate RDTSC better. It is currently an unreliable hack.
        dprintf(0, "vmm WARNING ::: rdtsc exit enabled in kernel vmx configuration.\n");
        dprintf(0, "                using hacky fallback implementation of tsc.\n");
        dprintf(0, "                you may find that Linux isn't too happy about this.\n");
        _vmm_debug_rdtsc_warning = true;
    }

    vmm_tsc_dword_t tsc = vmm_host_read_tsc();

    dprintf(5, "vmm exit RDTSC : returning TSC value %llu.\n", tsc.dword);

    /* Return the time value in guest registers. */
    guest->context.edx = tsc.high;
    guest->context.eax = tsc.low;
    guest->context.eip += guest->instruction_length;
    return LIB_VMM_SUCC;
}

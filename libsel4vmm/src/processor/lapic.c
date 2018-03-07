/* @TAG(OTHER_GPL) */

/*
 * Local APIC virtualization
 *
 * Copyright (C) 2006 Qumranet, Inc.
 * Copyright (C) 2007 Novell
 * Copyright (C) 2007 Intel
 * Copyright 2009 Red Hat, Inc. and/or its affiliates.
 *
 * Authors:
 *   Dor Laor <dor.laor@qumranet.com>
 *   Gregory Haskins <ghaskins@novell.com>
 *   Yaozu (Eddie) Dong <eddie.dong@intel.com>
 *
 * Based on Xen 3.1 code, Copyright (c) 2004, Intel Corporation.
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <utils/util.h>

#include "vmm/debug.h"

#include "vmm/processor/lapic.h"
#include "vmm/processor/apicdef.h"
#include "vmm/processor/msr.h"
#include "vmm/mmio.h"

#define APIC_BUS_CYCLE_NS 1

#define APIC_DEBUG 0
#define apic_debug(lvl,...) do{ if(lvl < APIC_DEBUG){printf(__VA_ARGS__);fflush(stdout);}}while (0)

#define APIC_LVT_NUM            6
/* 14 is the version for Xeon and Pentium 8.4.8*/
#define APIC_VERSION            (0x14UL | ((APIC_LVT_NUM - 1) << 16))
#define LAPIC_MMIO_LENGTH       (BIT(12))
/* followed define is not in apicdef.h */
#define APIC_SHORT_MASK         0xc0000
#define APIC_DEST_NOSHORT       0x0
#define APIC_DEST_MASK          0x800
#define MAX_APIC_VECTOR         256
#define APIC_VECTORS_PER_REG        32

inline static int pic_get_interrupt(vmm_t *vmm)
{
    return vmm->plat_callbacks.get_interrupt();
}

inline static int pic_has_interrupt(vmm_t *vmm)
{
    return vmm->plat_callbacks.has_interrupt();
}

struct vmm_lapic_irq {
    uint32_t vector;
    uint32_t delivery_mode;
    uint32_t dest_mode;
    uint32_t level;
    uint32_t trig_mode;
    uint32_t shorthand;
    uint32_t dest_id;
};

/* Generic bit operations; TODO move these elsewhere */
static inline int fls(int x)
{
    int r = 32;

    if (!x)
        return 0;

    if (!(x & 0xffff0000u)) {
        x <<= 16;
        r -= 16;
    }
    if (!(x & 0xff000000u)) {
        x <<= 8;
        r -= 8;
    }
    if (!(x & 0xf0000000u)) {
        x <<= 4;
        r -= 4;
    }
    if (!(x & 0xc0000000u)) {
        x <<= 2;
        r -= 2;
    }
    if (!(x & 0x80000000u)) {
        x <<= 1;
        r -= 1;
    }
    return r;
}

static uint32_t hweight32(unsigned int w)
{
    uint32_t res = w - ((w >> 1) & 0x55555555);
    res = (res & 0x33333333) + ((res >> 2) & 0x33333333);
    res = (res + (res >> 4)) & 0x0F0F0F0F;
    res = res + (res >> 8);
    return (res + (res >> 16)) & 0x000000FF;
}
/* End generic bit ops */

void vmm_lapic_reset(vmm_vcpu_t *vcpu);

// Returns whether the irq delivery mode is lowest prio
inline static bool vmm_is_dm_lowest_prio(struct vmm_lapic_irq *irq)
{
    return irq->delivery_mode == APIC_DM_LOWEST;
}

// Access register field
static inline void apic_set_reg(vmm_lapic_t *apic, int reg_off, uint32_t val)
{
    *((uint32_t *) (apic->regs + reg_off)) = val;
}

static inline uint32_t vmm_apic_get_reg(vmm_lapic_t *apic, int reg_off)
{
    return *((uint32_t *) (apic->regs + reg_off));
}

static inline int apic_test_vector(int vec, void *bitmap)
{
    return ((1UL << (vec & 31)) & ((uint32_t *)bitmap)[vec >> 5]) != 0;
}

bool vmm_apic_pending_eoi(vmm_vcpu_t *vcpu, int vector)
{
    vmm_lapic_t *apic = vcpu->lapic;

    return apic_test_vector(vector, apic->regs + APIC_ISR) ||
        apic_test_vector(vector, apic->regs + APIC_IRR);
}

static inline void apic_set_vector(int vec, void *bitmap)
{
    ((uint32_t *)bitmap)[vec >> 5] |= 1UL << (vec & 31);
}

static inline void apic_clear_vector(int vec, void *bitmap)
{
    ((uint32_t *)bitmap)[vec >> 5] &= ~(1UL << (vec & 31));
}

static inline int vmm_apic_sw_enabled(vmm_lapic_t *apic)
{
    return vmm_apic_get_reg(apic, APIC_SPIV) & APIC_SPIV_APIC_ENABLED;
}

static inline int vmm_apic_hw_enabled(vmm_lapic_t *apic)
{
    return apic->apic_base & MSR_IA32_APICBASE_ENABLE;
}

inline int vmm_apic_enabled(vmm_lapic_t *apic)
{
    return vmm_apic_sw_enabled(apic) && vmm_apic_hw_enabled(apic);
}

#define LVT_MASK    \
    (APIC_LVT_MASKED | APIC_SEND_PENDING | APIC_VECTOR_MASK)

#define LINT_MASK   \
    (LVT_MASK | APIC_MODE_MASK | APIC_INPUT_POLARITY | \
     APIC_LVT_REMOTE_IRR | APIC_LVT_LEVEL_TRIGGER)

static inline int vmm_apic_id(vmm_lapic_t *apic)
{
    return (vmm_apic_get_reg(apic, APIC_ID) >> 24) & 0xff;
}

static inline void apic_set_spiv(vmm_lapic_t *apic, uint32_t val)
{
    apic_set_reg(apic, APIC_SPIV, val);
}

static const unsigned int apic_lvt_mask[APIC_LVT_NUM] = {
    LVT_MASK ,      /* part LVTT mask, timer mode mask added at runtime */
    LVT_MASK | APIC_MODE_MASK,  /* LVTTHMR */
    LVT_MASK | APIC_MODE_MASK,  /* LVTPC */
    LINT_MASK, LINT_MASK,   /* LVT0-1 */
    LVT_MASK        /* LVTERR */
};

static inline void vmm_apic_set_id(vmm_lapic_t *apic, uint8_t id)
{
    apic_set_reg(apic, APIC_ID, id << 24);
}

static inline void vmm_apic_set_ldr(vmm_lapic_t *apic, uint32_t id)
{
    apic_set_reg(apic, APIC_LDR, id);
}

static inline int apic_lvt_enabled(vmm_lapic_t *apic, int lvt_type)
{
    return !(vmm_apic_get_reg(apic, lvt_type) & APIC_LVT_MASKED);
}

static inline int apic_lvt_vector(vmm_lapic_t *apic, int lvt_type)
{
    return vmm_apic_get_reg(apic, lvt_type) & APIC_VECTOR_MASK;
}

static inline int vmm_vcpu_is_bsp(vmm_vcpu_t *vcpu)
{
    return vcpu->vcpu_id == BOOT_VCPU;
}

static inline int apic_lvt_nmi_mode(uint32_t lvt_val)
{
    return (lvt_val & (APIC_MODE_MASK | APIC_LVT_MASKED)) == APIC_DM_NMI;
}

int vmm_apic_compare_prio(vmm_vcpu_t *vcpu1, vmm_vcpu_t *vcpu2)
{
    return vcpu1->lapic->arb_prio - vcpu2->lapic->arb_prio;
}

static void UNUSED dump_vector(const char *name, void *bitmap)
{
    int vec;
    uint32_t *reg = bitmap;

    printf("%s = 0x", name);

    for (vec = MAX_APIC_VECTOR - APIC_VECTORS_PER_REG;
            vec >= 0; vec -= APIC_VECTORS_PER_REG) {
        printf("%08x", reg[vec >> 5]);
    }

    printf("\n");
}

static int find_highest_vector(void *bitmap)
{
    int vec;
    uint32_t *reg = bitmap;

    for (vec = MAX_APIC_VECTOR - APIC_VECTORS_PER_REG;
         vec >= 0; vec -= APIC_VECTORS_PER_REG) {
        if (reg[vec >> 5])
            return fls(reg[vec >> 5]) - 1 + vec;
    }

    return -1;
}

static uint8_t UNUSED count_vectors(void *bitmap)
{
    int vec;
    uint32_t *reg = bitmap;
    uint8_t count = 0;

    for (vec = 0; vec < MAX_APIC_VECTOR; vec += APIC_VECTORS_PER_REG) {
        count += hweight32(reg[vec >> 5]);
    }

    return count;
}

static inline int apic_search_irr(vmm_lapic_t *apic)
{
    return find_highest_vector(apic->regs + APIC_IRR);
}

static inline int apic_find_highest_irr(vmm_lapic_t *apic)
{
    int result;

    if (!apic->irr_pending)
        return -1;

    result = apic_search_irr(apic);
    assert(result == -1 || result >= 16);

    return result;
}

static inline void apic_set_irr(int vec, vmm_lapic_t *apic)
{
    if (vec != 0x30) {
        apic_debug(5, "!settting irr 0x%x\n", vec);
    }

    apic->irr_pending = true;
    apic_set_vector(vec, apic->regs + APIC_IRR);
}

static inline void apic_clear_irr(int vec, vmm_lapic_t *apic)
{
    apic_clear_vector(vec, apic->regs + APIC_IRR);

    vec = apic_search_irr(apic);
    apic->irr_pending = (vec != -1);
}

static inline void apic_set_isr(int vec, vmm_lapic_t *apic)
{
    if (apic_test_vector(vec, apic->regs + APIC_ISR)) {
        return;
    }
    apic_set_vector(vec, apic->regs + APIC_ISR);

    ++apic->isr_count;
    /*
     * ISR (in service register) bit is set when injecting an interrupt.
     * The highest vector is injected. Thus the latest bit set matches
     * the highest bit in ISR.
     */
}

static inline int apic_find_highest_isr(vmm_lapic_t *apic)
{
    int result;

    /*
     * Note that isr_count is always 1, and highest_isr_cache
     * is always -1, with APIC virtualization enabled.
     */
    if (!apic->isr_count)
        return -1;
    if (apic->highest_isr_cache != -1)
        return apic->highest_isr_cache;

    result = find_highest_vector(apic->regs + APIC_ISR);
    assert(result == -1 || result >= 16);

    return result;
}

static inline void apic_clear_isr(int vec, vmm_lapic_t *apic)
{
    if (!apic_test_vector(vec, apic->regs + APIC_ISR)) {
        return;
    }
    apic_clear_vector(vec, apic->regs + APIC_ISR);

    --apic->isr_count;
    apic->highest_isr_cache = -1;
}

int vmm_lapic_find_highest_irr(vmm_vcpu_t *vcpu)
{
    int highest_irr;

    highest_irr = apic_find_highest_irr(vcpu->lapic);

    return highest_irr;
}

static int __apic_accept_irq(vmm_vcpu_t *vcpu, int delivery_mode,
                 int vector, int level, int trig_mode,
                 unsigned long *dest_map);

int vmm_apic_set_irq(vmm_vcpu_t *vcpu, struct vmm_lapic_irq *irq,
        unsigned long *dest_map)
{
    return __apic_accept_irq(vcpu, irq->delivery_mode, irq->vector,
            irq->level, irq->trig_mode, dest_map);
}

void vmm_apic_update_tmr(vmm_vcpu_t *vcpu, uint32_t *tmr)
{
    vmm_lapic_t *apic = vcpu->lapic;
    int i;

    for (i = 0; i < 8; i++)
        apic_set_reg(apic, APIC_TMR + 0x10 * i, tmr[i]);
}

static void apic_update_ppr(vmm_vcpu_t *vcpu)
{
    uint32_t tpr, isrv, ppr, old_ppr;
    int isr;
    vmm_lapic_t *apic = vcpu->lapic;

    old_ppr = vmm_apic_get_reg(apic, APIC_PROCPRI);
    tpr = vmm_apic_get_reg(apic, APIC_TASKPRI);
    isr = apic_find_highest_isr(apic);
    isrv = (isr != -1) ? isr : 0;

    if ((tpr & 0xf0) >= (isrv & 0xf0))
        ppr = tpr & 0xff;
    else
        ppr = isrv & 0xf0;

    apic_debug(6, "vlapic %p, ppr 0x%x, isr 0x%x, isrv 0x%x\n",
           apic, ppr, isr, isrv);

    if (old_ppr != ppr) {
        apic_set_reg(apic, APIC_PROCPRI, ppr);
        if (ppr < old_ppr) {
            /* Might have unmasked some pending interrupts */
            vmm_vcpu_accept_interrupt(vcpu);
        }
    }
}

static void apic_set_tpr(vmm_vcpu_t *vcpu, uint32_t tpr)
{
    apic_set_reg(vcpu->lapic, APIC_TASKPRI, tpr);
    apic_debug(3, "vcpu %d lapic TPR set to %d\n", vcpu->vcpu_id, tpr);
    apic_update_ppr(vcpu);
}

int vmm_apic_match_physical_addr(vmm_lapic_t *apic, uint16_t dest)
{
    return dest == 0xff || vmm_apic_id(apic) == dest;
}

int vmm_apic_match_logical_addr(vmm_lapic_t *apic, uint8_t mda)
{
    int result = 0;
    uint32_t logical_id;

    logical_id = GET_APIC_LOGICAL_ID(vmm_apic_get_reg(apic, APIC_LDR));

    switch (vmm_apic_get_reg(apic, APIC_DFR)) {
    case APIC_DFR_FLAT:
        if (logical_id & mda)
            result = 1;
        break;
    case APIC_DFR_CLUSTER:
        if (((logical_id >> 4) == (mda >> 0x4))
            && (logical_id & mda & 0xf))
            result = 1;
        break;
    default:
        apic_debug(1, "Bad DFR: %08x\n", vmm_apic_get_reg(apic, APIC_DFR));
        break;
    }

    return result;
}

int vmm_apic_match_dest(vmm_vcpu_t *vcpu, vmm_lapic_t *source,
               int short_hand, int dest, int dest_mode)
{
    int result = 0;
    vmm_lapic_t *target = vcpu->lapic;

    assert(target);
    switch (short_hand) {
    case APIC_DEST_NOSHORT:
        if (dest_mode == 0)
            /* Physical mode. */
            result = vmm_apic_match_physical_addr(target, dest);
        else
            /* Logical mode. */
            result = vmm_apic_match_logical_addr(target, dest);
        break;
    case APIC_DEST_SELF:
        result = (target == source);
        break;
    case APIC_DEST_ALLINC:
        result = 1;
        break;
    case APIC_DEST_ALLBUT:
        result = (target != source);
        break;
    default:
        apic_debug(2, "apic: Bad dest shorthand value %x\n",
               short_hand);
        break;
    }

    apic_debug(4, "target %p, source %p, dest 0x%x, "
           "dest_mode 0x%x, short_hand 0x%x",
           target, source, dest, dest_mode, short_hand);
    if (result) {
        apic_debug(4, " MATCH\n");
    } else {
        apic_debug(4, "\n");
    }

    return result;
}

int vmm_irq_delivery_to_apic(vmm_vcpu_t *src_vcpu, struct vmm_lapic_irq *irq, unsigned long *dest_map)
{
    int i, r = -1;
    vmm_lapic_t *src = src_vcpu->lapic;
    vmm_t *vmm = src_vcpu->vmm;

    vmm_vcpu_t *lowest = NULL;

    if (irq->shorthand == APIC_DEST_SELF) {
        return vmm_apic_set_irq(src_vcpu, irq, dest_map);
    }

    for (i = 0; i < vmm->num_vcpus; i++) {
        vmm_vcpu_t *dest_vcpu = &vmm->vcpus[i];

        if (!vmm_apic_hw_enabled(dest_vcpu->lapic)) {
            continue;
        }

        if (!vmm_apic_match_dest(dest_vcpu, src, irq->shorthand,
                        irq->dest_id, irq->dest_mode)) {
            continue;
        }

        if (!vmm_is_dm_lowest_prio(irq)) {
            // Normal delivery
            if (r < 0) {
                r = 0;
            }
            r += vmm_apic_set_irq(dest_vcpu, irq, dest_map);
        } else if (vmm_apic_enabled(dest_vcpu->lapic)) {
            // Pick vcpu with lowest priority to deliver to
            if (!lowest) {
                lowest = dest_vcpu;
            } else if (vmm_apic_compare_prio(dest_vcpu, lowest) < 0) {
                lowest = dest_vcpu;
            }
        }
    }

    if (lowest) {
        r = vmm_apic_set_irq(lowest, irq, dest_map);
    }

    return r;
}

/*
 * Add a pending IRQ into lapic.
 * Return 1 if successfully added and 0 if discarded.
 */
static int __apic_accept_irq(vmm_vcpu_t *vcpu, int delivery_mode,
                 int vector, int level, int trig_mode,
                 unsigned long *dest_map)
{
    int result = 0;
    vmm_lapic_t *apic = vcpu->lapic;

    switch (delivery_mode) {
    case APIC_DM_LOWEST:
        apic->arb_prio++;
    case APIC_DM_FIXED:
        /* FIXME add logic for vcpu on reset */
        if (!vmm_apic_enabled(apic)) {
            break;
        }
        apic_debug(4, "####fixed ipi 0x%x to vcpu %d\n", vector, vcpu->vcpu_id);

        result = 1;
        apic_set_irr(vector, apic);
        vmm_vcpu_accept_interrupt(vcpu);
        break;

    case APIC_DM_NMI:
    case APIC_DM_REMRD:
        result = 1;
        vmm_vcpu_accept_interrupt(vcpu);
        break;

    case APIC_DM_SMI:
        apic_debug(2, "Ignoring guest SMI\n");
        break;

    case APIC_DM_INIT:
        apic_debug(2, "Got init ipi on vcpu %d\n", vcpu->vcpu_id);
        if (!trig_mode || level) {
            if (apic->state == LAPIC_STATE_RUN) {
                /* Already running, ignore inits */
                break;
            }
            result = 1;
            vmm_lapic_reset(vcpu);
            apic->arb_prio = vmm_apic_id(apic);
            apic->state = LAPIC_STATE_WAITSIPI;
        } else {
            apic_debug(2, "Ignoring de-assert INIT to vcpu %d\n",
                   vcpu->vcpu_id);
        }
        break;

    case APIC_DM_STARTUP:
        if (apic->state != LAPIC_STATE_WAITSIPI) {
            apic_debug(1, "Received SIPI while processor was not in wait for SIPI state\n");
        } else {
            apic_debug(2, "SIPI to vcpu %d vector 0x%02x\n",
                   vcpu->vcpu_id, vector);
            result = 1;
            apic->sipi_vector = vector;
            apic->state = LAPIC_STATE_RUN;

            /* Start the VCPU thread. */
            vmm_start_ap_vcpu(vcpu, vector);
        }
        break;

    case APIC_DM_EXTINT:
        /* extints are handled by vmm_apic_consume_extints */
        printf("extint should not come to this function. vcpu %d\n", vcpu->vcpu_id);
        assert(0);
        break;

    default:
        printf("TODO: unsupported lapic ipi delivery mode %x", delivery_mode);
        assert(0);
        break;
    }
    return result;
}

static int apic_set_eoi(vmm_vcpu_t *vcpu)
{
    vmm_lapic_t *apic = vcpu->lapic;
    int vector = apic_find_highest_isr(apic);

    /*
     * Not every write EOI will has corresponding ISR,
     * one example is when Kernel check timer on setup_IO_APIC
     */
    if (vector == -1)
        return vector;

    apic_clear_isr(vector, apic);
    apic_update_ppr(vcpu);

    /* If another interrupt is pending, raise it */
    vmm_vcpu_accept_interrupt(vcpu);

    return vector;
}

static void apic_send_ipi(vmm_vcpu_t *vcpu)
{
    vmm_lapic_t *apic = vcpu->lapic;
    uint32_t icr_low = vmm_apic_get_reg(apic, APIC_ICR);
    uint32_t icr_high = vmm_apic_get_reg(apic, APIC_ICR2);
    struct vmm_lapic_irq irq;

    irq.vector = icr_low & APIC_VECTOR_MASK;
    irq.delivery_mode = icr_low & APIC_MODE_MASK;
    irq.dest_mode = icr_low & APIC_DEST_MASK;
    irq.level = icr_low & APIC_INT_ASSERT;
    irq.trig_mode = icr_low & APIC_INT_LEVELTRIG;
    irq.shorthand = icr_low & APIC_SHORT_MASK;
    irq.dest_id = GET_APIC_DEST_FIELD(icr_high);

    apic_debug(3, "icr_high 0x%x, icr_low 0x%x, "
           "short_hand 0x%x, dest 0x%x, trig_mode 0x%x, level 0x%x, "
           "dest_mode 0x%x, delivery_mode 0x%x, vector 0x%x\n",
           icr_high, icr_low, irq.shorthand, irq.dest_id,
           irq.trig_mode, irq.level, irq.dest_mode, irq.delivery_mode,
           irq.vector);

    vmm_irq_delivery_to_apic(vcpu, &irq, NULL);
}

static uint32_t __apic_read(vmm_lapic_t *apic, unsigned int offset)
{
    uint32_t val = 0;

    if (offset >= LAPIC_MMIO_LENGTH)
        return 0;

    switch (offset) {
    case APIC_ID:
        val = vmm_apic_id(apic) << 24;
        break;
    case APIC_ARBPRI:
        apic_debug(2, "Access APIC ARBPRI register which is for P6\n");
        break;

    case APIC_TMCCT:    /* Timer CCR */
        break;
    case APIC_PROCPRI:
        val = vmm_apic_get_reg(apic, offset);
        break;
    default:
        val = vmm_apic_get_reg(apic, offset);
        break;
    }

    return val;
}

static void apic_manage_nmi_watchdog(vmm_lapic_t *apic, uint32_t lvt0_val)
{
    int nmi_wd_enabled = apic_lvt_nmi_mode(vmm_apic_get_reg(apic, APIC_LVT0));

    if (apic_lvt_nmi_mode(lvt0_val)) {
        if (!nmi_wd_enabled) {
            apic_debug(4, "Receive NMI setting on APIC_LVT0 \n");
        }
    }
}

static int apic_reg_write(vmm_vcpu_t *vcpu, uint32_t reg, uint32_t val)
{
    vmm_lapic_t *apic = vcpu->lapic;
    int ret = 0;

    switch (reg) {
    case APIC_ID:       /* Local APIC ID */
        vmm_apic_set_id(apic, val >> 24);
        break;

    case APIC_TASKPRI:
        apic_set_tpr(vcpu, val & 0xff);
        break;

    case APIC_EOI:
        apic_set_eoi(vcpu);
        break;

    case APIC_LDR:
        vmm_apic_set_ldr(apic, val & APIC_LDR_MASK);
        break;

    case APIC_DFR:
        apic_set_reg(apic, APIC_DFR, val | 0x0FFFFFFF);
        break;

    case APIC_SPIV: {
        uint32_t mask = 0x3ff;
        if (vmm_apic_get_reg(apic, APIC_LVR) & APIC_LVR_DIRECTED_EOI)
            mask |= APIC_SPIV_DIRECTED_EOI;
        apic_set_spiv(apic, val & mask);
        if (!(val & APIC_SPIV_APIC_ENABLED)) {
            int i;
            uint32_t lvt_val;

            for (i = 0; i < APIC_LVT_NUM; i++) {
                lvt_val = vmm_apic_get_reg(apic,
                               APIC_LVTT + 0x10 * i);
                apic_set_reg(apic, APIC_LVTT + 0x10 * i,
                         lvt_val | APIC_LVT_MASKED);
            }
        //    atomic_set(&apic->lapic_timer.pending, 0);

        }
        break;
    }
    case APIC_ICR:
        /* No delay here, so we always clear the pending bit */
        apic_set_reg(apic, APIC_ICR, val & ~(BIT(12)));
        apic_send_ipi(vcpu);
        break;

    case APIC_ICR2:
        val &= 0xff000000;
        apic_set_reg(apic, APIC_ICR2, val);
        break;

    case APIC_LVT0:
        apic_manage_nmi_watchdog(apic, val);
    case APIC_LVTTHMR:
    case APIC_LVTPC:
    case APIC_LVT1:
    case APIC_LVTERR:
        /* TODO: Check vector */
        if (!vmm_apic_sw_enabled(apic))
            val |= APIC_LVT_MASKED;

        val &= apic_lvt_mask[(reg - APIC_LVTT) >> 4];
        apic_set_reg(apic, reg, val);

        break;

    case APIC_LVTT:
        apic_set_reg(apic, APIC_LVTT, val);
        break;

    case APIC_TMICT:
        apic_set_reg(apic, APIC_TMICT, val);
        break;

    case APIC_TDCR:
        apic_set_reg(apic, APIC_TDCR, val);
        break;

    default:
        ret = 1;
        break;
    }
    if (ret)
        apic_debug(2, "Local APIC Write to read-only register %x\n", reg);
    return ret;
}

void vmm_apic_mmio_write(vmm_vcpu_t *vcpu, void *cookie, uint32_t offset,
        int len, const uint32_t data)
{
    (void)cookie;

    /*
     * APIC register must be aligned on 128-bits boundary.
     * 32/64/128 bits registers must be accessed thru 32 bits.
     * Refer SDM 8.4.1
     */
    if (len != 4 || (offset & 0xf)) {
        apic_debug(1, "apic write: bad size=%d %x\n", len, offset);
        return;
    }

    /* too common printing */
    if (offset != APIC_EOI)
        apic_debug(6, "lapic mmio write at %s: offset 0x%x with length 0x%x, and value is "
               "0x%x\n", __func__, offset, len, data);

    apic_reg_write(vcpu, offset & 0xff0, data);
}

static int apic_reg_read(vmm_lapic_t *apic, uint32_t offset, int len,
        void *data)
{
    unsigned char alignment = offset & 0xf;
    uint32_t result;
    /* this bitmask has a bit cleared for each reserved register */
    static const uint64_t rmask = 0x43ff01ffffffe70cULL;

    if ((alignment + len) > 4) {
        apic_debug(2, "APIC READ: alignment error %x %d\n",
               offset, len);
        return 1;
    }

    if (offset > 0x3f0 || !(rmask & (1ULL << (offset >> 4)))) {
        apic_debug(2, "APIC_READ: read reserved register %x\n",
               offset);
        return 1;
    }

    result = __apic_read(apic, offset & ~0xf);

    switch (len) {
    case 1:
    case 2:
    case 4:
        memcpy(data, (char *)&result + alignment, len);
        break;
    default:
        apic_debug(2, "Local APIC read with len = %x, "
               "should be 1,2, or 4 instead\n", len);
        break;
    }
    return 0;
}

void vmm_apic_mmio_read(vmm_vcpu_t *vcpu, void *cookie, uint32_t offset,
        int len, uint32_t *data)
{
    vmm_lapic_t *apic = vcpu->lapic;
    (void)cookie;

    apic_reg_read(apic, offset, len, data);

    apic_debug(6, "lapic mmio read on vcpu %d, reg %08x = %08x\n", vcpu->vcpu_id, offset, *data);

    return;
}

void vmm_free_lapic(vmm_vcpu_t *vcpu)
{
    vmm_lapic_t *apic = vcpu->lapic;

    if (!apic)
        return;

    if (apic->regs) {
        free(apic->regs);
    }

    free(apic);
}

void vmm_lapic_set_base_msr(vmm_vcpu_t *vcpu, uint32_t value)
{
    apic_debug(2, "IA32_APIC_BASE MSR set to %08x on vcpu %d\n", value, vcpu->vcpu_id);

    if (!(value & MSR_IA32_APICBASE_ENABLE)) {
        printf("Warning! Local apic has been disabled by MSR on vcpu %d. "
               "This will probably not work!\n", vcpu->vcpu_id);
    }

    vcpu->lapic->apic_base = value;
}

uint32_t vmm_lapic_get_base_msr(vmm_vcpu_t *vcpu)
{
    uint32_t value = vcpu->lapic->apic_base;

    if (vmm_vcpu_is_bsp(vcpu)) {
        value |= MSR_IA32_APICBASE_BSP;
    } else {
        value &= ~MSR_IA32_APICBASE_BSP;
    }

    apic_debug(2, "Read from IA32_APIC_BASE MSR returns %08x on vcpu %d\n", value, vcpu->vcpu_id);

    return value;
}

void vmm_lapic_reset(vmm_vcpu_t *vcpu)
{
    vmm_lapic_t *apic;
    int i;

    apic_debug(4, "%s\n", __func__);

    assert(vcpu);
    apic = vcpu->lapic;
    assert(apic != NULL);

    /* Stop the timer in case it's a reset to an active apic */

    vmm_apic_set_id(apic, vcpu->vcpu_id); /* In agreement with ACPI code */
    apic_set_reg(apic, APIC_LVR, APIC_VERSION);

    for (i = 0; i < APIC_LVT_NUM; i++)
        apic_set_reg(apic, APIC_LVTT + 0x10 * i, APIC_LVT_MASKED);

    apic_set_reg(apic, APIC_DFR, 0xffffffffU);
    apic_set_spiv(apic, 0xff);
    apic_set_reg(apic, APIC_TASKPRI, 0);
    vmm_apic_set_ldr(apic, 0);
    apic_set_reg(apic, APIC_ESR, 0);
    apic_set_reg(apic, APIC_ICR, 0);
    apic_set_reg(apic, APIC_ICR2, 0);
    apic_set_reg(apic, APIC_TDCR, 0);
    apic_set_reg(apic, APIC_TMICT, 0);
    for (i = 0; i < 8; i++) {
        apic_set_reg(apic, APIC_IRR + 0x10 * i, 0);
        apic_set_reg(apic, APIC_ISR + 0x10 * i, 0);
        apic_set_reg(apic, APIC_TMR + 0x10 * i, 0);
    }
    apic->irr_pending = 0;
    apic->isr_count = 0;
    apic->highest_isr_cache = -1;
    apic_update_ppr(vcpu);

    vcpu->lapic->arb_prio = 0;

    apic_debug(4, "%s: vcpu=%p, id=%d, base_msr="
           "0x%016x\n", __func__,
           vcpu, vmm_apic_id(apic),
           apic->apic_base);

    if (vcpu->vcpu_id == BOOT_VCPU) {
        /* Bootstrap boot vcpu lapic in virtual wire mode */
        apic_set_reg(apic, APIC_LVT0,
             SET_APIC_DELIVERY_MODE(0, APIC_MODE_EXTINT));
        apic_set_reg(apic, APIC_SPIV, APIC_SPIV_APIC_ENABLED);

        assert(vmm_apic_sw_enabled(apic));
    } else {
        apic_set_reg(apic, APIC_SPIV, 0);
    }
}

int vmm_create_lapic(vmm_vcpu_t *vcpu, int enabled)
{
    vmm_lapic_t *apic;

    assert(vcpu != NULL);
    apic_debug(2, "apic_init %d\n", vcpu->vcpu_id);

    apic = malloc(sizeof(*apic));
    if (!apic)
        goto nomem;

    vcpu->lapic = apic;

    apic->regs = malloc(sizeof(struct local_apic_regs)); // TODO this is a page; allocate a page
    if (!apic->regs) {
        printf("malloc apic regs error for vcpu %x\n",
               vcpu->vcpu_id);
        goto nomem_free_apic;
    }

    if (enabled) {
        vmm_lapic_set_base_msr(vcpu, APIC_DEFAULT_PHYS_BASE | MSR_IA32_APICBASE_ENABLE);
    } else {
        vmm_lapic_set_base_msr(vcpu, APIC_DEFAULT_PHYS_BASE);
    }

    /* mainly init registers */
    vmm_lapic_reset(vcpu);

    return 0;
nomem_free_apic:
    free(apic);
nomem:
    return -1;
}

/* Return 1 if this vcpu should accept a PIC interrupt */
int vmm_apic_accept_pic_intr(vmm_vcpu_t *vcpu)
{
    vmm_lapic_t *apic = vcpu->lapic;
    uint32_t lvt0 = vmm_apic_get_reg(apic, APIC_LVT0);

    return ((lvt0 & APIC_LVT_MASKED) == 0 &&
        GET_APIC_DELIVERY_MODE(lvt0) == APIC_MODE_EXTINT &&
        vmm_apic_sw_enabled(vcpu->lapic));
}

/* Service an interrupt */
int vmm_apic_get_interrupt(vmm_vcpu_t *vcpu)
{
    vmm_lapic_t *apic = vcpu->lapic;
    int vector = vmm_apic_has_interrupt(vcpu);

    if (vector == 1) {
        return pic_get_interrupt(vcpu->vmm);
    } else if (vector == -1) {
        return -1;
    }

    apic_set_isr(vector, apic);
    apic_update_ppr(vcpu);
    apic_clear_irr(vector, apic);
    return vector;
}

/* Return which vector is next up for servicing */
int vmm_apic_has_interrupt(vmm_vcpu_t *vcpu)
{
    vmm_lapic_t *apic = vcpu->lapic;
    int highest_irr;

    if (vmm_apic_accept_pic_intr(vcpu) && pic_has_interrupt(vcpu->vmm)) {
        return 1;
    }

    highest_irr = apic_find_highest_irr(apic);
    if ((highest_irr == -1) ||
        ((highest_irr & 0xF0) <= vmm_apic_get_reg(apic, APIC_PROCPRI))) {
        return -1;
    }

    return highest_irr;
}

#if 0
int vmm_apic_local_deliver(vmm_vcpu_t *vcpu, int lvt_type)
{
    vmm_lapic_t *apic = vcpu->lapic;
    uint32_t reg = vmm_apic_get_reg(apic, lvt_type);
    int vector, mode, trig_mode;

    if (!(reg & APIC_LVT_MASKED)) {
        vector = reg & APIC_VECTOR_MASK;
        mode = reg & APIC_MODE_MASK;
        trig_mode = reg & APIC_LVT_LEVEL_TRIGGER;
        return __apic_accept_irq(vcpu, mode, vector, 1, trig_mode, NULL);
    }
    return 0;
}
#endif

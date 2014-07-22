/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */
/* Intel 8259 Programmable Interrupt Controller (PIC) emulator.
 *
 *     Authors:
 *         Qian Ge
 *         Adrian Danis
 *
 *     Fri 22 Nov 2013 04:24:32 EST
 */

#ifndef _LIB_VMM_I8259_H_ 
#define _LIB_VMM_I8259_H_

#include "vmm/helper.h"

#define VMM_IRQCHIP_PIC_MASTER   0
#define VMM_IRQCHIP_PIC_SLAVE    1

#define PIC_NUM_PINS 16
#define SELECT_PIC(irq) ((irq) < 8 ? VMM_IRQCHIP_PIC_MASTER : VMM_IRQCHIP_PIC_SLAVE)


static inline int __vmm_irq_line_state(unsigned long *irq_state,
                       int irq_source_id, int level)
{
    /* Logical OR for level trig interrupt. */
    if (level)
        __set_bit(irq_source_id, irq_state);
    else
        __clear_bit(irq_source_id, irq_state);

    return !!(*irq_state);
}

/* PIC Machine state. */
struct vmm_pic_state {
    unsigned char last_irr;        /* Edge detection */
    unsigned char irr;             /* Interrupt request register */
    unsigned char imr;             /* Interrupt mask register */
    unsigned char isr;             /* Interrupt service register */
    unsigned char priority_add;    /* Highest irq priority */
    unsigned char irq_base;
    unsigned char read_reg_select;
    unsigned char poll;
    unsigned char special_mask;
    unsigned char init_state;
    unsigned char auto_eoi;
    unsigned char rotate_on_auto_eoi;
    unsigned char special_fully_nested_mode;
    unsigned char init4;           /* True if 4 byte init */
    unsigned char elcr;            /* PIIX edge/trigger selection */
    unsigned char elcr_mask;
    unsigned char isr_ack;         /* Interrupt ack detection */
    struct vmm_pic *pics_state;
};

/* Struct containig PIC state for a Guest OS instance. */
struct vmm_pic {
    unsigned int wakeup_needed;
    unsigned int pending_acks;
    struct vmm_pic_state pics[2];  /* 0 is master pic, 1 is slave pic */
    int output;                    /* Intr from master PIC */
    void (*ack_notifier)(void *opaque, int irq);
    unsigned long irq_states[PIC_NUM_PINS];
};

void vmm_pic_init_state(void);

int vmm_pic_set_irq(int sender, int irq, int irq_source_id, int level);

void vmm_pic_clear_all(struct vmm_pic *s, int irq_source_id);

int vmm_pic_has_irq(int sender);

int vmm_pic_read_irq(int sender);

#endif /* _LIB_VMM_I8259_H_ */



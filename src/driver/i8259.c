/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */
/* Intel 8259 Programmable Interrupt Controller (PIC) emulator on x86.
 *
 * The functions related to machine state manipulations were taken
 * from Linux kernel 3.8.8 arch/x86/kvm/i8259.c
 *
 *     Authors:
 *         Qian Ge
 */

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <sel4/sel4.h>

#include "vmm/config.h"
#include "vmm/vmm.h"
#include "vmm/io.h"
#include "vmm/driver/i8259.h"
#include "vmm/helper.h"

/* PIC machine state for guest OS. */
struct vmm_pic vmm_pic_gs[LIB_VMM_GUEST_NUMBER]; 

/* Return the highest priority found in mask (highest = smallest number). Return 8 if no irq */
static inline int get_priority(struct vmm_pic_state *s, int mask) {
    int priority = 0;

    if (!mask)
        return 8;

    while (!(mask & (1 << ((priority + s->priority_add) & 7))))
        priority++;
    return priority;
}

/* Check if given IO address is valid. */
static int vmm_pic_in_range(unsigned int addr) {
    switch (addr) {
        case 0x20:
        case 0x21:
        case 0xa0:
        case 0xa1:
        case 0x4d0:
        case 0x4d1:
            return 1;
        default:
            return 0;
    }
}


/* Compare ISR with the highest priority IRQ in IRR.
 *    Returns -1 if no interrupts,
 *    Otherwise returns the PIC interrupt generated.
 */
static int pic_get_irq(struct vmm_pic_state *s) {
    int mask, cur_priority, priority;

    mask = s->irr & ~s->imr;
    priority = get_priority(s, mask);
    if (priority == 8)
        return -1;
    /* Compute current priority. If special fully nested mode on the
     * master, the IRQ coming from the slave is not taken into account
     * for the priority computation.
     */
    mask = s->isr;
    if (s->special_fully_nested_mode && s == &s->pics_state->pics[0])
        mask &= ~(1 << 2);
    cur_priority = get_priority(s, mask);
    if (priority < cur_priority) {
        /* Higher priority found: an irq should be generated. */
        return (priority + s->priority_add) & 7;
    }
    else
        return -1;
}

/* Clear the IRQ from ISR, the IRQ has been served. */
static void pic_clear_isr(struct vmm_pic_state *s, int irq) {
    int error;
    /* Clear the ISR, notify the ack handler. */
    s->isr &= ~(1 << irq);
    if (s != &s->pics_state->pics[0])
        irq += 8;

    if (irq != 2) {
        error = seL4_IRQHandler_Ack(LIB_VMM_DRIVER_INTERRUPT_HANDLER_START + irq);
        assert(error == seL4_NoError);
    }
}

/* Set irq level. If an edge is detected, then the IRR is set to 1. */
static inline int pic_set_irq1(struct vmm_pic_state *s, int irq, int level) {
    int mask, ret = 1;
    mask = 1 << irq;
    
    if (s->elcr & mask) {
        /* Level triggered. */
        if (level) {
            ret = !(s->irr & mask);
            s->irr |= mask;
            s->last_irr |= mask;
        } else {
            s->irr &= ~mask;
            s->last_irr &= ~mask;
        }
    } else {
        /* Edge triggered. */
        if (level) {
            if ((s->last_irr & mask) == 0) {
                ret = !(s->irr & mask);
                s->irr |= mask;
            }
            s->last_irr |= mask;
        } else {
            s->last_irr &= ~mask;
        }
    }

    return (s->imr & mask) ? -1 : ret;
}


/* Raise IRQ on CPU if necessary. Must be called every time the active IRQ may change.
   Update the master pic and trigger interrupt injection according to the IRR and ISR. */
static void pic_update_irq(struct vmm_pic *s) {
    int irq2, irq;

    irq2 = pic_get_irq(&s->pics[1]);
    if (irq2 >= 0) {
        /* If IRQ request by slave PIC, signal master PIC and set the IRR in master PIC. */
        pic_set_irq1(&s->pics[0], 2, 1);
        pic_set_irq1(&s->pics[0], 2, 0);
    }
    irq = pic_get_irq(&s->pics[0]);

    /* PIC status changed injection flag. */
    if (!s->output)
        s->wakeup_needed = true;

    if (irq >= 0)
        s->output = 1;
    else
        s->output = 0;

    seL4_Notify(LIB_VMM_HOST_ASYNC_MESSAGE_CAP, 0);
}

/* Reset the PIC state for a guest OS. */
static void pic_reset(struct vmm_pic_state *s) {
    int irq;
    unsigned char edge_irr = s->irr & ~s->elcr;

    s->last_irr = 0;
    s->irr &= s->elcr;
    s->imr = 0;
    s->priority_add = 0;
    s->special_mask = 0;
    s->read_reg_select = 0;
    if (!s->init4) {
        s->special_fully_nested_mode = 0;
        s->auto_eoi = 0;
    }
    s->init_state = 1;

#if 0
    /* FIXME: CONNECT pic with APIC */
    kvm_for_each_vcpu(i, vcpu, s->piics_state->kvm)
        if (kvm_apic_accept_pic_intr(vcpu)) {
            found = true;
            break;
        }


    if (!found)
        return;
#endif

    for (irq = 0; irq < PIC_NUM_PINS/2; irq++) {
        if (edge_irr & (1 << irq)) {
            pic_clear_isr(s, irq);
        }
    }
}

/* Write into the state owned by the guest OS. */
static void pic_ioport_write(struct vmm_pic_state *s, unsigned int addr, unsigned int val) {
    int priority, cmd, irq;

    addr &= 1;

    if (addr == 0) {
        if (val & 0x10) {
            /* ICW1 */
            s->init4 = val & 1;
            if (val & 0x02)
                dprintf(1, "PIC: single mode not supported\n");
            if (val & 0x08)
                dprintf(1, "PIC: level sensitive irq not supported\n");
            /* Reset the machine state and pending IRQS. */
            pic_reset(s);
        } else if (val & 0x08) {
            /* OCW 3 */
            if (val & 0x04)
                s->poll = 1;
            if (val & 0x02)
                s->read_reg_select = val & 1;
            if (val & 0x40)
                s->special_mask = (val >> 5) & 1;
        } else {
            /* OCW 2 */
            cmd = val >> 5;
            switch (cmd) {
                case 0:
                case 4:
                    s->rotate_on_auto_eoi = cmd >> 2;
                    break;
                case 1:
                    /* End of interrupt. */
                case 5:
                    /* Clear ISR and update IRQ*/
                    priority = get_priority(s, s->isr);
                    if (priority != 8) {
                        irq = (priority + s->priority_add) & 7;
                        if (cmd == 5)
                            s->priority_add = (irq + 1) & 7;
                        pic_clear_isr(s, irq);
                        pic_update_irq(s->pics_state);
                    }
                    break;
                case 3:
                    /* Specific EOI command. */
                    irq = val & 7;
                    pic_clear_isr(s, irq);
                    pic_update_irq(s->pics_state);
                    break;
                case 6:
                    /* Set priority command. */
                    s->priority_add = (val + 1) & 7;
                    pic_update_irq(s->pics_state);
                    break;
                case 7:
                    /* Rotate on specific eoi command. */
                    irq = val & 7;
                    s->priority_add = (irq + 1) & 7;
                    pic_clear_isr(s, irq);
                    pic_update_irq(s->pics_state);
                    break;
                default:
                    /* No operation. */
                    break;
            }
        }
    } else
        switch (s->init_state) {
            case 0: { /* Normal mode OCW 1. */
                        unsigned char imr_diff = s->imr ^ val;
                        (void) imr_diff;
                        //off = (s == &s->pics_state->pics[0]) ? 0 : 8;
                        s->imr = val;
#if 0
                        for (irq = 0; irq < PIC_NUM_PINS/2; irq++)
                            if (imr_diff & (1 << irq))
                                /*FIXME: notify the status changes for IMR*/
                                kvm_fire_mask_notifiers(
                                        s->pics_state->kvm,
                                        SELECT_PIC(irq + off),
                                        irq + off,
                                        !!(s->imr & (1 << irq)));
#endif
                        pic_update_irq(s->pics_state);
                        break;
                    }
            case 1:
                    /* ICW 2 */
                    s->irq_base = val & 0xf8;
                    s->init_state = 2;
                    break;
            case 2:
                    if (s->init4)
                        s->init_state = 3;
                    else
                        s->init_state = 0;
                    break;
            case 3:
                    /* ICW 4 */
                    s->special_fully_nested_mode = (val >> 4) & 1;
                    s->auto_eoi = (val >> 1) & 1;
                    s->init_state = 0;
                    break;
        }
}

/* Poll the pending IRQS for the highest priority IRQ, ack the IRQ: clear the ISR and IRR, and
 * update PIC state. Returns -1 if no pending IRQ. */
static unsigned int pic_poll_read(struct vmm_pic_state *s, unsigned int addr1) {
    unsigned int ret;

    ret = pic_get_irq(s);

    if (ret >= 0) {
        if (addr1 >> 7) {
            s->pics_state->pics[0].isr &= ~(1 << 2);
            s->pics_state->pics[0].irr &= ~(1 << 2);
        }
        s->irr &= ~(1 << ret);
        pic_clear_isr(s, ret);
        if (addr1 >> 7 || ret != 2)
            pic_update_irq(s->pics_state);
    } else {
        ret = 0x07;
        pic_update_irq(s->pics_state);
    }

    return ret;
}


/* Read and write functions for PIC (master and slave). */
static unsigned int pic_ioport_read(struct vmm_pic_state *s, unsigned int addr) {
    unsigned int ret;

    /* Poll for the highest priority IRQ. */
    if (s->poll) {
        ret = pic_poll_read(s, addr);
        s->poll = 0;

    } else {
        if (!(addr & 1)) {
            if (s->read_reg_select)
                ret = s->isr;
            else
                ret = s->irr;
        }
        else
            ret = s->imr;

    }
    return ret;
}




/*read and write functions for ELCR  (edge/level control registers)
IO: 0x4d0 0x4d1 each bit corresponsing to an IRQ from 8259 
bit set: level triggered mode 
bit clear: edge triggered mode*/
static void elcr_ioport_write(struct vmm_pic_state *s, unsigned int addr, unsigned int val) {
    s->elcr = val & s->elcr_mask;
}

static unsigned int elcr_ioport_read(struct vmm_pic_state *s, unsigned int addr) {
    return s->elcr;
}


static int vmm_pic_write(seL4_Word sender, io_msg_t *recv_msg) {
    unsigned int addr = recv_msg->port_no;

    /* Sender thread is the VMM main thread, calculate guest ID according to the badge. */
    struct vmm_pic *s = vmm_pic_gs + VMM_GUEST_BADGE_TO_ID(sender);

    if (!vmm_pic_in_range(addr))
        return LIB_VMM_ERR;

    if (recv_msg->size != 1) {
        return LIB_VMM_ERR;
    }

    /* 0x20, 0x21, master pic, 0xa0, 0xa1 slave PIC. */
    switch (addr) {
        case 0x20:
        case 0x21:
        case 0xa0:
        case 0xa1:
            pic_ioport_write(&s->pics[addr >> 7], addr, recv_msg->val);
            break;
        case 0x4d0:
        case 0x4d1:
            elcr_ioport_write(&s->pics[addr & 1], addr, recv_msg->val);
            break;
    }

    return LIB_VMM_SUCC;
}

static int vmm_pic_read(seL4_Word sender, io_msg_t *recv_msg, unsigned int *val) {
    unsigned int addr = recv_msg->port_no;

    /* Sender thread is the VMM main thread, calculate guest ID according to the badge. */
    struct vmm_pic *s = vmm_pic_gs + VMM_GUEST_BADGE_TO_ID(sender);

    if (!vmm_pic_in_range(addr))
        return LIB_VMM_ERR;

    if (recv_msg->size != 1) {
        return LIB_VMM_ERR;
    }

    /* 0x20, 0x21, master pic, 0xa0, 0xa1 slave PIC. */
    switch (addr) {
        case 0x20:
        case 0x21:
        case 0xa0:
        case 0xa1:
            *val = pic_ioport_read(&s->pics[addr >> 7], addr);
            break;
        case 0x4d0:
        case 0x4d1:
            *val = elcr_ioport_read(&s->pics[addr & 1], addr);
            break;
    }
    return LIB_VMM_SUCC;
}

unsigned int vmm_pic_msg_proc(seL4_Word sender, unsigned int len) {
    io_msg_t recv_msg, send_msg;
    unsigned int ret = sizeof (send_msg);

    vmm_get_msg((void*)&recv_msg, len);

    send_msg.result = LIB_VMM_ERR;

    dprintf(3, "pic driver recv msg in %d, port 0x%x, size %d\n", recv_msg.in, recv_msg.port_no, recv_msg.size);

    if (recv_msg.in)  
        send_msg.result = vmm_pic_read(sender, &recv_msg, &send_msg.val);
    else 
        send_msg.result = vmm_pic_write(sender, &recv_msg);

    dprintf(3, "pic driver reply msg ret %d val 0x%x\n", send_msg.result, send_msg.val);

    vmm_put_msg((void *)&send_msg, sizeof (send_msg));
    return ret;
}


/* Init internal status for PIC driver. */
void vmm_pic_init_state(void) {
    struct vmm_pic *s;

    /* Init pic machine state for guest OS. */
    for (int i = 0; i < LIB_VMM_GUEST_NUMBER; i++) {
 
        s = vmm_pic_gs + i;

//        s->pics[0].elcr = seL4_IA32_IOPort_In8(LIB_VMM_IO_PCI_CAP, 0x4d0).result;
//        s->pics[1].elcr = seL4_IA32_IOPort_In8(LIB_VMM_IO_PCI_CAP, 0x4d1).result;
        s->pics[0].elcr = 0;
        s->pics[1].elcr = 0;
        s->pics[0].elcr_mask = 0xf8;
        s->pics[1].elcr_mask = 0xde;
        s->pics[0].pics_state = s;
        s->pics[1].pics_state = s;
    }
}


/* To inject an IRQ: First set the level as 1, then set the level as 0, toggling the level for
 * triggering the IRQ.
 * IRQ source ID is used for mapping multiple IRQ source into a IRQ pin.
 * Sets irq request into the state machine for PIC.
 */
int vmm_pic_set_irq(int sender, int irq, int irq_source_id, int level) {
    int ret, irq_level;

    struct vmm_pic *s = vmm_pic_gs + VMM_GUEST_BADGE_TO_ID(sender);

    irq_level = __vmm_irq_line_state(&s->irq_states[irq],
            irq_source_id, level);
   
    /* Set IRR. */
    ret = pic_set_irq1(&s->pics[irq >> 3], irq & 7, irq_level);
    pic_update_irq(s);

    return ret;
}

void vmm_pic_clear_all(struct vmm_pic *s, int irq_source_id)
{
    int i;

    for (i = 0; i < PIC_NUM_PINS; i++)
        __clear_bit(irq_source_id, &s->irq_states[i]);
}

/* Acknowledge interrupt IRQ. */
static inline void pic_intack(struct vmm_pic_state *s, int irq)
{
    /* Ack the IRQ, set the ISR. */
    s->isr |= 1 << irq;

    /* We don't clear a level sensitive interrupt here. */
    if (!(s->elcr & (1 << irq)))
        s->irr &= ~(1 << irq);

    /* Clear the ISR for auto EOI mode. */
    if (s->auto_eoi) {
        if (s->rotate_on_auto_eoi)
            s->priority_add = (irq + 1) & 7;
        pic_clear_isr(s, irq);
    }
}

/* Use output as a flag for pending IRQ. */
int vmm_pic_has_irq(int sender) {
    struct vmm_pic *s = vmm_pic_gs + VMM_GUEST_BADGE_TO_ID(sender);
    return s->output;
}


/* Return the highest pending IRQ. Ack the IRQ by updating the ISR before entering guest, using this
 * function to get the pending IRQ. */
int vmm_pic_read_irq(int sender)
{
    struct vmm_pic *s = vmm_pic_gs + VMM_GUEST_BADGE_TO_ID(sender);

    int irq, irq2, intno;

    /* Search for the highest priority IRQ. */
    irq = pic_get_irq(&s->pics[0]);

    if (irq >= 0) {
        /* Ack the IRQ. */
        pic_intack(&s->pics[0], irq);

        /* Ack the slave 8259 controller. */
        if (irq == 2) {
            irq2 = pic_get_irq(&s->pics[1]);
            if (irq2 >= 0)
                pic_intack(&s->pics[1], irq2);
            else
                /* Spurious IRQ on slave controller. */
                irq2 = 7;
            intno = s->pics[1].irq_base + irq2;
            irq = irq2 + 8;
        } else {
            intno = s->pics[0].irq_base + irq;
        }
    } else {
        /* Spurious IRQ on host 8259 controller. */
        irq = 7;
        intno = s->pics[0].irq_base + irq;
    }
    pic_update_irq(s);

    return intno;
}

void vmm_wakeup_guest(struct vmm_pic *s) {

    if (!s->wakeup_needed) 
        return;

    s->wakeup_needed = false;

    /* Wake up the guest OS, or switch the guest PS into host mode.
       if there is a pending IRQ, switch into host mode for injection. */
    seL4_Notify(LIB_VMM_HOST_ASYNC_MESSAGE_CAP, 0);
}


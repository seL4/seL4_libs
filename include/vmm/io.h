/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef _LIB_VMM_IO_H_
#define _LIB_VMM_IO_H_

#include <simple/simple.h>

/* IO ports in x86 system. */

/*first programmable interrupt controller, master*/
#define X86_IO_PIC_1_START   0x20 
#define X86_IO_PIC_1_END     0x21

/*programmable interval timer*/
#define X86_IO_PIT_START      0x40 
#define X86_IO_PIT_END        0x5f 

/*PS/2 controller*/
#define X86_IO_PS2C_START      0x60 
#define X86_IO_PS2C_END        0x64

/*read time clock / CMOS, NMI mask*/
#define X86_IO_RTC_START      0x70 
#define X86_IO_RTC_END        0x7f

/*ISA DMA*/
#define X86_IO_ISA_DMA_START      0x80 
#define X86_IO_ISA_DMA_END        0x8f

/*second programmable interrupt controller, slave*/
#define X86_IO_PIC_2_START    0xa0
#define X86_IO_PIC_2_END      0xa1 

/*POS Programmable Option Select (PS/2)*/
#define X86_IO_POS_START      0x100
#define X86_IO_POS_END        0x10F

/*serial port 2*/
#define X86_IO_SERIAL_2_START  0x2f8 
#define X86_IO_SERIAL_2_END    0x2ff

/*serial port 3*/
#define X86_IO_SERIAL_3_START  0x3e8 
#define X86_IO_SERIAL_3_END    0x3ef

/*serial port 1*/
#define X86_IO_SERIAL_1_START  0x3f8 
#define X86_IO_SERIAL_1_END    0x3ff

/*serial port 4*/
#define X86_IO_SERIAL_4_START  0x2e8 
#define X86_IO_SERIAL_4_END    0x2ef

/*ELCR (edge/level control register) for IRQ line*/
#define X86_IO_ELCR_START      0x4d0 
#define X86_IO_ELCR_END        0x4d1 


/*pci configuration space*/
#define X86_IO_PCI_CONFIG_START 0xcf8 
#define X86_IO_PCI_CONFIG_END   0xcff 

/*invalid IO port no*/
#define X86_IO_INVALID          0xfff

/*enable passthrough mode*/
#define X86_IO_PASSTHROUGH 1
#define X86_IO_CATCH 0

#define LABEL_VMM_GUEST_IO_MESSAGE 0
#define LABEL_VMM_HOST_IO_INITGUEST 1

/*io msg format, communicating to drivers*/
typedef struct io_msg {
    unsigned int in;           /*IN/OUT*/
    unsigned int port_no;
    unsigned int size;         /*1/2/4 bytes*/
    unsigned int val;
    int result;                /*LIB_VMM_SUCC/LIB_VMM_ERR*/
    unsigned int sender;
} io_msg_t;

typedef struct ioport_range {
    unsigned int port_start;
    unsigned int port_end;

    seL4_CPtr cap_no;
    char passthrough_mode;
 
    const char* desc;
} ioport_range_t;

struct thread_rec;

/* Internal interfaces related with IO ports. */
void vmm_pic_init_state(void);
unsigned int vmm_pic_msg_proc(seL4_Word sender, unsigned int len);

void vmm_io_init_guest(simple_t *simple, struct thread_rec *guest, seL4_CPtr vcpu, seL4_Word badgen);

#endif

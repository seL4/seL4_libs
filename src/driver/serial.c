/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */
/* Minimal hacky serial device emulation (currently unused).
 *     Authors:
 *         Qian Ge
 *         Xi Ma Chen
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sel4/sel4.h>

#include "vmm/vmm.h"
#include "vmm/config.h"
#include "vmm/helper.h"
#include "vmm/io.h"
#include "vmm/driver/serial.h"

typedef struct serial_drv_state {
    unsigned int REG[8];
} serial_drv_state_t;

static serial_drv_state_t* serial_state;
static serial_drv_state_t default_serial_state;

static void serial_init_state(serial_drv_state_t* s) {
    assert(s);
    memset(s, 0, sizeof(serial_drv_state_t));
    s->REG[5] = 0x20; // Transmit always ready.
}

static void serial_put_char(char c) {
#ifdef SEL4_DEBUG_KERNEL
    printf("%c", c);
#else
    seL4_IA32_IOPort_In8_t res;
    
    /* Wait for serial to become ready. */
    do {
        res = seL4_IA32_IOPort_In8(LIB_VMM_IO_SERIAL_CAP, CONSOLE(LSR));
        assert(res.error == 0);
    } while (!(res.result & (1 << 5)));
    
    /* Write out the next character. */
    seL4_IA32_IOPort_Out8(LIB_VMM_IO_SERIAL_CAP, CONSOLE(THR), c);
#endif

    if(c == '\n')
        serial_put_char('\r');
}

/* Handle serial device IOPort messages and emulate device. */
static io_msg_t serial_msg_proc(seL4_Word sender, unsigned int len) {
    assert(serial_state);
    assert(len == sizeof(io_msg_t)); //We expect an IPC from vmm io.

    io_msg_t reply;
    memset(&reply, 0, sizeof(io_msg_t));

    io_msg_t m;
    vmm_get_msg(&m, len);
    assert(m.port_no >= X86_IO_SERIAL_1_START && m.port_no <= X86_IO_SERIAL_1_END);

    unsigned int reg = m.port_no - X86_IO_SERIAL_1_START;
    unsigned int val = m.val & 0xff;

    dprintf(4, "serial_msg_proc: in %d port 0x%x size %d val 0x%x reg %d\n", m.in, m.port_no, m.size, val, reg);
    assert(reg >= 0 && reg < 8);

    if (m.size != 1) {
        dprintf(3, "serial_msg_proc: WARNING unsupported size %d\n", m.size);
        reply.result = LIB_VMM_ERR;
        return reply;
    }

    switch (reg) {
    case 0: // Data register.
        if (!m.in && !(serial_state->REG[3] & 0x80)) {
            serial_put_char(val);
        }
        // Fallthrough
    default:
        // Ignore.
        if (m.in) {
            reply.result = LIB_VMM_SUCC;
            reply.size = 1;
            reply.val = serial_state->REG[reg];
        } else {
            serial_state->REG[reg] = val;
        }
        break;
    }

    return reply;

}



/* The entry function for serial module. */
void vmm_driver_serial_run(void) {
    serial_state = &default_serial_state;
    serial_init_state(serial_state);

    seL4_MessageInfo_t msg;
    seL4_Word sender;
    unsigned int len; 
    seL4_MessageInfo_t reply_info = seL4_MessageInfo_new(0, 0, 0, sizeof(io_msg_t));

    dprintf(2, "VMM Virtual serial driver started.\n");

    while (1) {
        /* Wait for incoming serial msg. */
        msg = seL4_Wait(LIB_VMM_DRIVER_SERIAL_CAP, &sender);  
        len = seL4_MessageInfo_get_length(msg);

        io_msg_t reply = serial_msg_proc(sender, len);
        
        /* Reply to the sender. */
        vmm_put_msg(&reply, sizeof(io_msg_t));
        seL4_Reply(reply_info);
    }
}


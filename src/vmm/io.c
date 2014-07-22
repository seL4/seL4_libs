/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/*vm exits related with io instructions*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include <sel4/sel4.h>
#include <sel4utils/util.h>


#include "vmm/config.h"
#include "vmm/helper.h"
#include "vmm/io.h"
#include "vmm/platform/guest_devices.h"
#include "vmm/platform/sys.h"
#include "vmm/vmcs.h"
#include "vmm/vmexit.h"
#include "vmm/vmm.h"

#include <simple/simple.h>
#include <simple-stable/simple-stable.h>

/* Debug helper function for port no. */
#ifdef LIB_VMM_DEBUG
static const char* vmm_debug_io_portno_desc(int port_no) {
    const char* desc = "Unknown IO Port";
    for (int i = 0; ioport_global_map[i].port_start != X86_IO_INVALID; i++) {
        if (ioport_global_map[i].port_start <= port_no && ioport_global_map[i].port_end >= port_no) {
            desc = ioport_global_map[i].desc;
            break;
        }
    }
    return desc;
}
#endif

/* Send message to host VMM thread and wait for reply. */
static inline void vmm_io_sw_msg(seL4_CPtr cap_no, void *send, unsigned int len, void *recv) {
    seL4_MessageInfo_t send_msg = seL4_MessageInfo_new(LABEL_VMM_GUEST_IO_MESSAGE, 0, 0, len);
    seL4_MessageInfo_t recv_msg; 
    unsigned int recv_len;

    vmm_put_msg(send, len);

    recv_msg = seL4_Call(cap_no, send_msg);
    recv_len = seL4_MessageInfo_get_length(recv_msg);

    vmm_get_msg(recv, recv_len);
}

static void vmm_io_printout(io_msg_t send) {
    dprintf(0, "vm exit io ERROR: string %d  in %d rep %d  port no 0x%x (%s) size %d\n", 0,
            send.in, 0, send.port_no, vmm_debug_io_portno_desc(send.port_no), send.size);
}

/* IO instruction execution handler. */
int vmm_io_instruction_handler(gcb_t *guest) {

    unsigned int exit_qualification = guest->qualification;
    unsigned int string, rep;
    int ret = LIB_VMM_ERR;
    seL4_CPtr cap_no = LIB_VMM_CAP_INVALID;
    bool cap_found = false;
    io_msg_t send, recv;

    string = (exit_qualification & 16) != 0;
    send.sender = guest->sender;
    send.in = (exit_qualification & 8) != 0;
    send.port_no = exit_qualification >> 16;
    send.size = (exit_qualification & 7) + 1;
    rep = (exit_qualification & 0x20) >> 5;

    dprintf(4, "vm exit io request: string %d  in %d rep %d  port no 0x%x (%s) size %d\n", string,
            send.in, rep, send.port_no, vmm_debug_io_portno_desc(send.port_no), send.size);

    /*FIXME: does not support string and rep instructions*/
    if (string || rep) {
        dprintf(0, "vm exit io request: FIXME: does not support string and rep instructions");
        vmm_io_printout(send);
        return ret;
    }

    if (!send.in) {
        send.val = guest->context.eax;
        if (send.size < 4)
            send.val &= MASK(send.size * 8);
        dprintf(4, "vm exit io request: out instruction val 0x%x\n", send.val);
    }

    /* Find the right cap for IO request*/
    for (int i = 0; ioport_global_map[i].port_start != X86_IO_INVALID; i++) {
        if (ioport_global_map[i].port_start <= send.port_no && ioport_global_map[i].port_end >= send.port_no) {
            cap_no = ioport_global_map[i].cap_no;
            cap_found = true;
            break;
        }
    }

    #ifndef LIB_VMM_IGNORE_UNDEFINED_IOPORTS
    if (!cap_found) {
        dprintf(0, "vm exit io request: WARNING - cap not found for port 0x% (%s)\n", send.port_no,
                vmm_debug_io_portno_desc(send.port_no));
        vmm_io_printout(send);
        printf("EIP = 0x%x\n", guest->context.eip);
        return ret;
    }
    #endif
    
    if (!cap_found || cap_no == LIB_VMM_CAP_INVALID) {
        /* ignore unsupported IO request. */
        static int last_port = -1;
        if (last_port != send.port_no) {
            dprintf(0, "vm exit io request: WARNING - ignoring unsupported ioport 0x%x (%s)\n", send.port_no,
                    vmm_debug_io_portno_desc(send.port_no));
            last_port = send.port_no;
        }
        if (send.in) {
            if ( send.size < 4) {
                guest->context.eax |= MASK(send.size * 8);
            } else {
                guest->context.eax = -1;
            }
        }
        guest->context.eip += guest->instruction_length;
        return LIB_VMM_SUCC;
    }

    vmm_io_sw_msg(cap_no, (void *)&send, sizeof (send), (void *)&recv);

    if (recv.result == LIB_VMM_ERR) {
        dprintf(0, "vm exit io request: handler returned error.");
        vmm_io_printout(send);
        return ret;
    }

    /*IN EAX, PORT NO*/
    if (send.in) {
        /* mask out the correct size from eax */
        if (send.size < 4) {
            guest->context.eax &= ~MASK(send.size * 8);
        } else {
            guest->context.eax = 0;
        }
        guest->context.eax |= recv.val;
        dprintf(4, "vm exit io respond: in instruction val 0x%x\n", recv.val);
    }
    guest->context.eip += guest->instruction_length;

    return LIB_VMM_SUCC;
}

/*configure io ports for a guest*/
void vmm_io_init_guest(simple_t *simple, thread_rec_t *guest, seL4_CPtr vcpu, seL4_Word badgen) {
    int error;
    assert(simple);
    assert(vcpu);
    assert(guest);

    /* Initialise IO ports (defaults to all fault and no passthrough). */
    error = seL4_IA32_VCPU_SetIOPort(vcpu, 
        simple_get_IOPort_cap(simple, 0, /* all 2^16 io ports */(1<<16)-1));
    assert(error == seL4_NoError);

    /* Set global IO port settings. */
    for (int i = 0; ioport_global_map[i].port_start != X86_IO_INVALID; i++) {
        dprintf(1, "vmm io: setting %s IO port 0x%x --> 0x%x to %s.\n", 
                ioport_global_map[i].desc, ioport_global_map[i].port_start,
                ioport_global_map[i].port_end,
                ioport_global_map[i].passthrough_mode ? "PASSTHROUGH" : "CATCH_FAULT");

        if (ioport_global_map[i].passthrough_mode != X86_IO_PASSTHROUGH) {
            // We don't need to do anything here, as SetIOPort defaults to catch.
            continue;
        }

        error = seL4_IA32_VCPU_SetIOPortMask(vcpu,
                ioport_global_map[i].port_start, ioport_global_map[i].port_end, 0);
        assert(error == seL4_NoError);
    }

    /* Set per-guest IO port settings. */
    for (int i = 0; guest->guest_ioport_map[i].port_start != X86_IO_INVALID; i++) {
        dprintf(1, "vmm io: setting guest %s IO port 0x%x --> 0x%x to %s.\n", 
                guest->guest_ioport_map[i].desc, guest->guest_ioport_map[i].port_start,
                guest->guest_ioport_map[i].port_end,
                guest->guest_ioport_map[i].passthrough_mode ? "PASSTHROUGH" : "CATCH_FAULT");

        if (guest->guest_ioport_map[i].passthrough_mode != X86_IO_PASSTHROUGH) {
            // We don't need to do anything here, as SetIOPort defaults to catch.
            continue;
        }

        error = seL4_IA32_VCPU_SetIOPortMask(vcpu,
                guest->guest_ioport_map[i].port_start, guest->guest_ioport_map[i].port_end, 0);
        assert(error == seL4_NoError);
    }
}


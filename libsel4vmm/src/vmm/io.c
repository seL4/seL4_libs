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

/*vm exits related with io instructions*/

#include <stdio.h>
#include <stdlib.h>

#include <sel4/sel4.h>
#include <sel4utils/util.h>
#include <simple/simple.h>

#include "vmm/debug.h"
#include "vmm/io.h"
#include "vmm/vmm.h"

static int io_port_cmp(const void *pkey, const void *pelem) {
    unsigned int key = (unsigned int)(uintptr_t)pkey;
    const ioport_range_t *elem = (const ioport_range_t*)pelem;
    if (key < elem->port_start) {
        return -1;
    }
    if (key > elem->port_end) {
        return 1;
    }
    return 0;
}

static int io_port_cmp2(const void *a, const void *b) {
    const ioport_range_t *aa = (const ioport_range_t*) a;
    const ioport_range_t *bb = (const ioport_range_t*) b;
    return aa->port_start - bb->port_start;
}

static ioport_range_t *search_port(vmm_io_port_list_t *io_port, unsigned int port_no) {
    return (ioport_range_t*)bsearch((void*)(uintptr_t)port_no, io_port->ioports, io_port->num_ioports, sizeof(ioport_range_t), io_port_cmp);
}

/* Debug helper function for port no. */
static const char* vmm_debug_io_portno_desc(vmm_io_port_list_t *io_port, int port_no) {
    ioport_range_t *port = search_port(io_port, port_no);
    return port ? port->desc : "Unknown IO Port";
}

/* IO instruction execution handler. */
int vmm_io_instruction_handler(vmm_vcpu_t *vcpu) {

    unsigned int exit_qualification = vmm_guest_exit_get_qualification(&vcpu->guest_state);
    unsigned int string, rep;
    int ret;
    unsigned int port_no;
    unsigned int size;
    unsigned int value;
    int is_in;

    string = (exit_qualification & 16) != 0;
    is_in = (exit_qualification & 8) != 0;
    port_no = exit_qualification >> 16;
    size = (exit_qualification & 7) + 1;
    rep = (exit_qualification & 0x20) >> 5;

    DPRINTF(4, "vm exit io request: string %d  in %d rep %d  port no 0x%x (%s) size %d\n", string,
            is_in, rep, port_no, vmm_debug_io_portno_desc(&vcpu->vmm->io_port, port_no), size);

    /*FIXME: does not support string and rep instructions*/
    if (string || rep) {
        DPRINTF(0, "vm exit io request: FIXME: does not support string and rep instructions");
        DPRINTF(0, "vm exit io ERROR: string %d  in %d rep %d  port no 0x%x (%s) size %d\n", 0,
                is_in, 0, port_no, vmm_debug_io_portno_desc(&vcpu->vmm->io_port, port_no), size);
        return -1;
    }

    ioport_range_t *port = search_port(&vcpu->vmm->io_port, port_no);
    if (!port) {
        static int last_port = -1;
        if (last_port != port_no) {
            DPRINTF(3, "vm exit io request: WARNING - ignoring unsupported ioport 0x%x (%s)\n", port_no,
                    vmm_debug_io_portno_desc(&vcpu->vmm->io_port, port_no));
            last_port = port_no;
        }
        if (is_in) {
            uint32_t eax;
            if ( size < 4) {
                eax = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EAX);
                eax |= MASK(size * 8);
            } else {
                eax = -1;
            }
            vmm_set_user_context(&vcpu->guest_state, USER_CONTEXT_EAX, eax);
        }
        vmm_guest_exit_next_instruction(&vcpu->guest_state, vcpu->guest_vcpu);
        return 0;
    }

    if (is_in) {
        uint32_t eax;
        ret = port->port_in(port->cookie, port_no, size, &value);
        if (size < 4) {
            eax = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EAX);
            eax &= ~MASK(size * 8);
            eax |= value;
        } else {
            eax = value;
        }
        vmm_set_user_context(&vcpu->guest_state, USER_CONTEXT_EAX, eax);
    } else {
        value = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EAX);
        if (size < 4)
            value &= MASK(size * 8);
        ret = port->port_out(port->cookie, port_no, size, value);
    }

    if (ret) {
        ZF_LOGE("vm exit io request: handler returned error.");
        ZF_LOGE("vm exit io ERROR: string %d  in %d rep %d  port no 0x%x (%s) size %d", 0,
                is_in, 0, port_no, vmm_debug_io_portno_desc(&vcpu->vmm->io_port, port_no), size);
        return -1;
    }

    vmm_guest_exit_next_instruction(&vcpu->guest_state, vcpu->guest_vcpu);

    return 0;
}

static int add_io_port_range(vmm_io_port_list_t *io_list, ioport_range_t port) {
    /* ensure this range does not overlap */
    for (int i = 0; i < io_list->num_ioports; i++) {
        if (io_list->ioports[i].port_end >= port.port_start && io_list->ioports[i].port_start <= port.port_end) {
            ZF_LOGE("Requested ioport range 0x%x-0x%x for %s overlaps with existing range 0x%x-0x%x for %s",
                port.port_start, port.port_end, port.desc ? port.desc : "Unknown IO Port", io_list->ioports[i].port_start, io_list->ioports[i].port_end, io_list->ioports[i].desc ? io_list->ioports[i].desc : "Unknown IO Port");
            return -1;
        }
    }
    /* grow the array */
    io_list->ioports = realloc(io_list->ioports, sizeof(ioport_range_t) * (io_list->num_ioports + 1));
    assert(io_list->ioports);
    /* add the new entry */
    io_list->ioports[io_list->num_ioports] = port;
    io_list->num_ioports++;
    /* sort */
    qsort(io_list->ioports, io_list->num_ioports, sizeof(ioport_range_t), io_port_cmp2);
    return 0;
}

int vmm_io_port_add_passthrough(vmm_io_port_list_t *io_list, uint16_t start, uint16_t end, const char *desc) {
    return add_io_port_range(io_list, (ioport_range_t){start, end, 1, NULL, NULL, NULL, desc});
}

/* Add an io port range for emulation */
int vmm_io_port_add_handler(vmm_io_port_list_t *io_list, uint16_t start, uint16_t end, void *cookie, ioport_in_fn port_in, ioport_out_fn port_out, const char *desc) {
    return add_io_port_range(io_list, (ioport_range_t){start, end, 0, cookie, port_in, port_out, desc});
}

/*configure io ports for a guest*/
int vmm_io_port_init_guest(vmm_io_port_list_t *io_list, simple_t *simple, seL4_CPtr vcpu) {
    int UNUSED error;

    for (int i = 0; i < io_list->num_ioports; i++) {
        ioport_range_t *port = &io_list->ioports[i];
        if (port->passthrough) {
            DPRINTF(1, "vmm io port: setting %s IO port 0x%x - 0x%x to passthrough\n", port->desc, port->port_start, port->port_end);
            seL4_CPtr ioport = simple_get_IOPort_cap(simple, port->port_start, port->port_end);
            if (!ioport) {
                ZF_LOGE("Failed to get \"%s\" io port from simple for range 0x%x - 0x%x", port->desc, port->port_start, port->port_end);
                return -1;
            }
            error = seL4_X86_VCPU_EnableIOPort(vcpu, ioport, port->port_start, port->port_end);
            assert(error == seL4_NoError);
        }
    }

    return 0;
}

int vmm_io_port_init(vmm_io_port_list_t *io_list) {
    io_list->num_ioports = 0;
    io_list->ioports = malloc(0);
    assert(io_list->ioports);
    return 0;
}

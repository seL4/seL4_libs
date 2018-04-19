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

#pragma once

#include <stdint.h>

#include <sel4/sel4.h>
#include <simple/simple.h>

typedef int (*ioport_in_fn)(void *cookie, unsigned int port_no, unsigned int size, unsigned int *result);
typedef int (*ioport_out_fn)(void *cookie, unsigned int port_no, unsigned int size, unsigned int value);

typedef struct ioport_range {
    unsigned int port_start;
    unsigned int port_end;

    int passthrough;

    /* If not passthrough, then handler functions */
    void *cookie;
    ioport_in_fn port_in;
    ioport_out_fn port_out;

    const char* desc;
} ioport_range_t;

typedef struct vmm_io_list {
    int num_ioports;
    /* Sorted list of ioport functions */
    ioport_range_t *ioports;
} vmm_io_port_list_t;

/* Initialize the io port list manager */
int vmm_io_port_init(vmm_io_port_list_t *io_list);

/* Add an io port range for pass through */
int vmm_io_port_add_passthrough(vmm_io_port_list_t *io_list, uint16_t start, uint16_t end, const char *desc);

/* Add an io port range for emulation */
int vmm_io_port_add_handler(vmm_io_port_list_t *io_list, uint16_t start, uint16_t end, void *cookie, ioport_in_fn port_in, ioport_out_fn port_out, const char *desc);

/* Add io ports to guest vcpu */
int vmm_io_port_init_guest(vmm_io_port_list_t *io_list, simple_t *simple, seL4_CPtr vcpu, vka_t *vka);


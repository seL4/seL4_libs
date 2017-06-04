/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#pragma once

#include <sel4platsupport/plat/serial.h>
#include <sel4platsupport/arch/serial.h>

#include <simple/simple.h>
#include <vka/vka.h>
#include <vka/object.h>
#include <vspace/vspace.h>

typedef struct serial_objects {
    /* Path for the default serial irq handler */
    cspacepath_t serial_irq_path;
    arch_serial_objects_t arch_serial_objects;
} serial_objects_t;

/**
 * Creates caps required for using default serial devices and stores them
 * in supplied serial_objects_t.
 *
 * Uses the provided vka, vspace and simple interfaces for creating the caps.
 * (A vspace is required because on some platforms devices may need to be mapped in
 *  order to query hardware features.)
 *
 * @param  vka            Allocator to allocate objects with
 * @param  vspace         vspace for mapping device frames into memory if necessary.
 * @param  simple         simple interface for access to init caps
 * @param  serial_objects struct for returning cap meta data.
 * @return                0 on success, otherwise failure.
 */
int sel4platsupport_init_default_serial_caps(vka_t *vka, vspace_t *vspace, simple_t *simple, serial_objects_t *serial_objects);
int sel4platsupport_arch_init_default_serial_caps(vka_t *vka, vspace_t *vspace, simple_t *simple, serial_objects_t *serial_objects);

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __SEL4PLATSUPPORT_PLATSUPPORT_H__
#define __SEL4PLATSUPPORT_PLATSUPPORT_H__

#include <vspace/vspace.h>
#include <simple/simple.h>
#include <vka/vka.h>

/* performs a legacy failsafe initialisation of the serial port. This will stomp
 * over your virtual address space in unpredictable ways and is generally not
 * recommended */
void
platsupport_serial_setup_bootinfo_failsafe(void);

/* Initialises the serial device. Requires a simple_t * so that caps can be located as well as a vka_t to allocate at slot, and a vspace in case a memory mapped device is required */
void
platsupport_serial_setup_simple(vspace_t *vspace, simple_t *simple, vka_t *vka);

/* Do not look too closely at this function. Exists as shortest path to sanity in sel4test */
void
platsupport_zero_globals(void);

void
platsupport_undo_serial_setup(void);

void
platsupport_serial_input_init_IRQ(void);

#endif /* __SEL4PLATSUPPORT_PLATSUPPORT_H__ */

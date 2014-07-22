/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _SEL4_PLATSUPPORT_PIT_H_
#define _SEL4_PLATSUPPORT_PIT_H_

#include <sel4/sel4.h>

#include <vka/vka.h>
#include <simple/simple.h>
#include <platsupport/io.h>
#include <sel4platsupport/timer.h>
#include <platsupport/plat/pit.h>

#define DEFAULT_TIMER_PADDR 0
#define DEFAULT_TIMER_INTERRUPT PIT_INTERRUPT
#define PIT_IO_PORT_MIN 0x40
#define PIT_IO_PORT_MAX 0x43

/*
 * Get the PIT interface.
 *
 * @param aep endpoint for PIT irqs to come in on. 
 * @return NULL on error. 
 */
seL4_timer_t* sel4platsupport_get_pit(vka_t *vka, simple_t *simple, ps_io_port_ops_t *ops, seL4_CPtr aep);

/*
 * Stop the timer and free allocated resources
 */
void sel4platsupport_destroy_pit(seL4_timer_t *pit, vka_t *vka);
#endif /* _SEL4_PLATSUPPORT_PIT_H_ */

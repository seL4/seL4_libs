/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef _IA32_PLATSUPPORT_IO_H
#define _IA32_PLATSUPPORT_IO_H

#include <sel4/sel4.h>
#include <platsupport/io.h>
#include <simple/simple.h>

/*
 * Get an io_port_ops interface.
 *
 * @param ops pointer to the interface struct to fill in
 * @param simple Simple interface that will be used to retrieve IOPort capabilities. This pointer
 *               needs to remain valid for as long as you use the filled in interface
 *
 * @return 0 on success.
 */
int sel4platsupport_get_io_port_ops(ps_io_port_ops_t *ops, simple_t *simple);


#endif /* _IA32_PLATSUPPORT_IO_H */


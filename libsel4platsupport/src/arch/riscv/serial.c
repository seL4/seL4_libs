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

#include <sel4platsupport/serial.h>
#include <sel4platsupport/device.h>

int
sel4platsupport_arch_init_default_serial_caps(UNUSED vka_t *vka, UNUSED vspace_t *vspace, 
											  UNUSED simple_t *simple, UNUSED serial_objects_t *serial_objects)
{
    /* No serial driver */
    ZF_LOGE("Not implemented");
    return -1;
}

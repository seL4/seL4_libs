/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __SEL4PLATSUPPORT_ARCH_DEVICE_H__
#define __SEL4PLATSUPPORT_ARCH_DEVICE_H__

#include <sel4/sel4.h>
#include <sel4platsupport/arch/device.h>
#include <simple/simple.h>
#include <vka/vka.h>

/*
 * Allocate a cslot for an msi irq and get the cap for an irq.
 
 * @param vka to allocate slot with
 * @param simple to get the cap from
 * @param irq_number to get the cap to
 * @param[out] dest empty path struct to return path to irq in 
 * @return 0 on success
 */
seL4_Error sel4platsupport_copy_msi_cap(vka_t *vka, simple_t *simple, seL4_Word irq_number, 
                                        cspacepath_t *dest);
#endif /* __SEL4PLATSUPPORT_ARCH_DEVICE_H__ */

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __SEL4PLATSUPPORT_DEVICE_H__
#define __SEL4PLATSUPPORT_DEVICE_H__

#include <sel4/sel4.h>

/**
 * Parse a device cap from boot info.
 *
 * @param paddr     physical address of the device
 * @param page_bits page size in bits of the device page
 *
 * @return 0 on failure, otherwise the cap.
 */
seL4_CPtr
platsupport_find_device_cap(seL4_Word paddr, seL4_Word page_bits);

#endif /* __SEL4PLATSUPPORT_DEVICE_H__ */

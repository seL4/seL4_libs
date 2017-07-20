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

#include <sel4platsupport/device.h>
#include <utils/util.h>

int sel4platsupport_arch_copy_irq_cap(UNUSED arch_simple_t *arch_simple, UNUSED ps_irq_t *irq,
        UNUSED cspacepath_t *dest)
{
    ZF_LOGE("unknown irq type");
    return -1;
}

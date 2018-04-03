/*
 * Copyright 2018, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */
#include <autoconf.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <sel4/sel4.h>

#include <simple-default/simple-default.h>

#include <vspace/page.h>

seL4_Error simple_default_get_irq(void *data, int irq, seL4_CNode root, seL4_Word index, uint8_t depth) {
    return seL4_IRQControl_Get(seL4_CapIRQControl, irq, root, index, depth);
}

void
simple_default_init_arch_simple(arch_simple_t *simple, void *data) 
{
    simple->data = data;
    simple->irq = simple_default_get_irq;
}


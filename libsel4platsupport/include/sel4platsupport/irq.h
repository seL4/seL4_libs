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

#include <vka/vka.h>
#include <platsupport/irq.h>

typedef struct sel4ps_irq {
    cspacepath_t handler_path;
    cspacepath_t badged_ntfn_path;
    ps_irq_t irq;
} sel4ps_irq_t;

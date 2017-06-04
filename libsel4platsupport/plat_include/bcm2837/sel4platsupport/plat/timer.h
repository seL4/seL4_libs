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

#include <sel4platsupport/timer_types.h>

#include <platsupport/plat/timer.h>

#define DEFAULT_TIMER_INTERRUPT SP804_TIMER_IRQ

#define DEFAULT_TIMER_PADDR    SP804_TIMER_PADDR

typedef struct plat_timer_objects {
} plat_timer_objects_t;

#include <sel4platsupport/arch/timer.h>
#include <sel4platsupport/timer.h>

/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#pragma once
#include <sel4platsupport/timer_types.h>

#include <platsupport/plat/timer.h>

#define DEFAULT_TIMER_INTERRUPT INT_NV_TMR1
#define DEFAULT_TIMER_PADDR    NV_TMR_PADDR

typedef struct plat_timer_objects {
} plat_timer_objects_t;

#include <sel4platsupport/arch/timer.h>
#include <sel4platsupport/timer.h>

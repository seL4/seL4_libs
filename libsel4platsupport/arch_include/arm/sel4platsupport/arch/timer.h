/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#pragma once

#include <sel4platsupport/timer_types.h>
#include <sel4platsupport/plat/timer.h>

#include <autoconf.h>
#include <platsupport/timer.h>
#include <platsupport/plat/timer.h>
#include <vspace/vspace.h>
#include <simple/simple.h>
#include <vka/vka.h>

typedef struct arch_timer_objects {
    plat_timer_objects_t plat_timer_objects;
} arch_timer_objects_t;

#ifdef CONFIG_ARCH_ARM_V7A
#ifdef CONFIG_ARM_CORTEX_A15

seL4_timer_t *sel4platsupport_get_generic_timer(void);

#include <sel4platsupport/timer.h>

#endif /* CONFIG_ARM_CORTEX_A15 */
#endif /* CONFIG_ARCH_ARM_V7A */

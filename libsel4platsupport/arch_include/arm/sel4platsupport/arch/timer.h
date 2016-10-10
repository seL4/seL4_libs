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

#include <autoconf.h>
#include <sel4platsupport/timer.h>
#include <platsupport/timer.h>
#include <platsupport/plat/timer.h>

#ifdef CONFIG_ARCH_ARM_V7A
#ifdef CONFIG_ARM_CORTEX_A15

seL4_timer_t *sel4platsupport_get_generic_timer(void);

#endif /* CONFIG_ARM_CORTEX_A15 */
#endif /* CONFIG_ARCH_ARM_V7A */

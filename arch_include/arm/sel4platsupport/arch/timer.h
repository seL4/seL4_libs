/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef __SEL4_PLAT_SUPPORT_ARCH_TIMER_H
#define __SEL4_PLAT_SUPPORT_ARCH_TIMER_H

#include <sel4platsupport/timer.h>
#include <platsupport/timer.h>
#include <platsupport/plat/timer.h>

#ifdef CONFIG_ARCH_ARM_V7A
#ifdef CONFIG_ARM_CORTEX_A15

seL4_timer_t *sel4platsupport_get_generic_timer(void);

#endif /* CONFIG_ARM_CORTEX_A15 */
#endif /* CONFIG_ARCH_ARM_V7A */

#endif /* __SEL4_PLAT_SUPPORT_ARCH_TIMER_H */

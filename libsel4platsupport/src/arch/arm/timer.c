/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <sel4platsupport/timer.h>
#include <platsupport/arch/generic_timer.h>

#include <stdlib.h>

#ifdef CONFIG_ARCH_ARM_V7A
#ifdef CONFIG_ARM_CORTEX_A15

seL4_timer_t *
sel4platsupport_get_generic_timer(void)
{

    seL4_timer_t *timer = calloc(1, sizeof(seL4_timer_t));
    if (timer == NULL) {
        return NULL;
    }

    timer->timer = generic_timer_get_timer();
    return timer;
}

#endif /* CONFIG_ARM_CORTEX_A15 */
#endif /* CONFIG_ARCH_ARM_V7A */


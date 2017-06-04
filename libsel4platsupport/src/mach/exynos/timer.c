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

#include <vka/vka.h>
#include <vspace/vspace.h>
#include <simple/simple.h>
#include <sel4platsupport/plat/timer.h>

int sel4platsupport_plat_init_default_timer_caps(UNUSED vka_t *vka, UNUSED vspace_t *vspace, UNUSED simple_t *simple, UNUSED timer_objects_t *timer_objects)
{
    return 0;
}

seL4_timer_t *sel4platsupport_get_default_timer(vka_t *vka, vspace_t *vspace, simple_t *simple, seL4_CPtr notification)
{
    return sel4platsupport_get_pwm(vspace, simple, vka, notification);
}

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <vka/vka.h>
#include <vspace/vspace.h>
#include <simple/simple.h>
#include <sel4platsupport/plat/timer.h>


seL4_timer_t *sel4platsupport_get_default_timer(vka_t *vka, vspace_t *vspace, simple_t *simple, seL4_CPtr aep)
{
    return sel4platsupport_get_gpt(vspace, simple, vka, aep, GPT1, 1);
}

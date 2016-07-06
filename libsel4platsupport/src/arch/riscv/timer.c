/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#include <autoconf.h>
#include <vka/vka.h>
#include <vspace/vspace.h>

#include <sel4platsupport/timer.h>
#include <sel4platsupport/plat/timer.h>

#include <stdlib.h>

seL4_timer_t *
sel4platsupport_get_generic_timer(void)
{
    return NULL;
}

uintptr_t
sel4platsupport_get_default_timer_paddr(UNUSED vka_t *vka, UNUSED vspace_t *vspace)
{
    return NULL;
}

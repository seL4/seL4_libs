/*
 * Copyright 2018, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
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
    return 0;
}

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <platsupport/io.h>
#include <vka/vka.h>
#include <vspace/vspace.h>
#include <sel4platsupport/plat/timer.h>
#include <sel4platsupport/arch/io.h>
#include <simple/simple.h>
#include <utils/util.h>

static ps_io_port_ops_t ops;

seL4_timer_t *sel4platsupport_get_default_timer(vka_t *vka, UNUSED vspace_t *vspace,
                                                simple_t *simple, seL4_CPtr aep)
{

    UNUSED int error = sel4platsupport_get_io_port_ops(&ops, simple);
    assert(error == 0);

    seL4_timer_t *pit = sel4platsupport_get_pit(vka, simple, &ops, aep);
    assert(pit != NULL);

    return pit;
}

seL4_timer_t *
sel4platsupport_get_tsc_timer(seL4_timer_t *timeout_timer)
{

    seL4_timer_t *timer = calloc(1, sizeof(seL4_timer_t));
    if (timer == NULL) {
        return NULL;
    }

    timer->timer = tsc_get_timer(timeout_timer->timer);

    return timer;
}




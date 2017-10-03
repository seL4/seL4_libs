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

#include <sel4/sel4.h>
#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <sel4utils/util.h>
#include <sel4utils/time_server/client.h>
#include <utils/util.h>

typedef struct {
    seL4_CPtr ep;
    seL4_Word label;
} client_ltimer_t;

static int client_get_time(void *data, uint64_t *time)
{
    client_ltimer_t *ltimer = data;
    seL4_MessageInfo_t info = seL4_MessageInfo_new(ltimer->label, 0, 0, 1);
    seL4_SetMR(0, GET_TIME);
    seL4_Call(ltimer->ep, info);
    *time = sel4utils_64_get_mr(1);
    return seL4_GetMR(0);
}

static int client_set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    client_ltimer_t *ltimer = data;
    seL4_MessageInfo_t info = seL4_MessageInfo_new(ltimer->label, 0, 0, 2 + SEL4UTILS_64_WORDS);
    seL4_SetMR(0, SET_TIMEOUT);
    seL4_SetMR(1, type);
    sel4utils_64_set_mr(2, ns);
    info = seL4_Call(ltimer->ep, info);
    return seL4_GetMR(0);
}

int sel4utils_rpc_ltimer_init(ltimer_t *ltimer, ps_io_ops_t ops, seL4_CPtr ep, seL4_Word label)
{
    ltimer->get_time = client_get_time;
    ltimer->set_timeout = client_set_timeout;

    int error = ps_calloc(&ops.malloc_ops, 1, sizeof(client_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);
    client_ltimer_t *client_ltimer = ltimer->data;
    client_ltimer->ep = ep;
    client_ltimer->label = label;

    /* success! */
    return 0;
}

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
#include <platsupport/io.h>
#include <sel4platsupport/io.h>
#include <sel4platsupport/arch/io.h>
#include <assert.h>
#include <stdint.h>
#include <utils/util.h>

static int
sel4platsupport_io_port_in(void *cookie, uint32_t port, int io_size, uint32_t *result)
{
    simple_t *simple = (simple_t*)cookie;
    uint32_t last_port = port + io_size - 1;
    seL4_X86_IOPort io_port_cap = simple_get_IOPort_cap(simple, port, last_port);
    if (!io_port_cap) {
        ZF_LOGE("Failed to get capability for IOPort range 0x%x-0x%x", port, last_port);
        return -1;
    }

    switch (io_size) {
    case 1: {
        seL4_X86_IOPort_In8_t x = seL4_X86_IOPort_In8(io_port_cap, port);
        *result = x.result;
        return x.error;
    }
    case 2: {
        seL4_X86_IOPort_In16_t x = seL4_X86_IOPort_In16(io_port_cap, port);
        *result = x.result;
        return x.error;
    }
    case 4: {
        seL4_X86_IOPort_In32_t x = seL4_X86_IOPort_In32(io_port_cap, port);
        *result = x.result;
        return x.error;
    }
    default:
        ZF_LOGE("Invalid io_size %d, expected 1, 2 or 4", io_size);
        return -1;
    }

}

static int
sel4platsupport_io_port_out(void *cookie, uint32_t port, int io_size, uint32_t val)
{
    simple_t *simple = (simple_t*)cookie;
    uint32_t last_port = port + io_size - 1;
    seL4_X86_IOPort io_port_cap = simple_get_IOPort_cap(simple, port, last_port);
    if (!io_port_cap) {
        ZF_LOGE("Failed to get capability for IOPort range 0x%x-0x%x", port, last_port);
        return -1;
    }

    switch (io_size) {
    case 1:
        return seL4_X86_IOPort_Out8(io_port_cap, port, val);
    case 2:
        return seL4_X86_IOPort_Out16(io_port_cap, port, val);
    case 4:
        return seL4_X86_IOPort_Out32(io_port_cap, port, val);
    default:
        ZF_LOGE("Invalid io_size %d, expected 1, 2 or 4", io_size);
        return -1;
    }

}

int
sel4platsupport_get_io_port_ops(ps_io_port_ops_t *ops, simple_t *simple)
{
    assert(ops != NULL);
    assert(simple != NULL);
    ops->io_port_in_fn = sel4platsupport_io_port_in;
    ops->io_port_out_fn = sel4platsupport_io_port_out;
    ops->cookie = (void *) simple;

    return 0;
}

int sel4platsupport_new_arch_ops(ps_io_ops_t *ops, simple_t *simple)
{
    if (!ops || !simple) {
        return EINVAL;
    }

    return sel4platsupport_get_io_port_ops(&ops->io_port_ops, simple);
}

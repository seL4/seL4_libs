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
#include <vka/capops.h>

typedef struct io_cookie {
    simple_t *simple;
    vka_t *vka;
} io_cookie_t;

static int
sel4platsupport_io_port_in(void *cookie, uint32_t port, int io_size, uint32_t *result)
{
    io_cookie_t *io_cookie = cookie;
    uint32_t last_port = port + io_size - 1;
    cspacepath_t path;
    int error;
    error = vka_cspace_alloc_path(io_cookie->vka, &path);
    if (error) {
        ZF_LOGE("Failed to allocate slot");
        return error;
    }
    error = simple_get_IOPort_cap(io_cookie->simple, port, last_port, path.root, path.capPtr, path.capDepth);
    if (error) {
        ZF_LOGE("Failed to get capability for IOPort range 0x%x-0x%x", port, last_port);
        return -1;
    }

    switch (io_size) {
    case 1: {
        seL4_X86_IOPort_In8_t x = seL4_X86_IOPort_In8(path.capPtr, port);
        *result = x.result;
        error = x.error;
        break;
    }
    case 2: {
        seL4_X86_IOPort_In16_t x = seL4_X86_IOPort_In16(path.capPtr, port);
        *result = x.result;
        error = x.error;
        break;
    }
    case 4: {
        seL4_X86_IOPort_In32_t x = seL4_X86_IOPort_In32(path.capPtr, port);
        *result = x.result;
        error = x.error;
        break;
    }
    default:
        ZF_LOGE("Invalid io_size %d, expected 1, 2 or 4", io_size);
        error = -1;
        break;
    }

    vka_cnode_delete(&path);
    vka_cspace_free_path(io_cookie->vka, path);
    return error;
}

static int
sel4platsupport_io_port_out(void *cookie, uint32_t port, int io_size, uint32_t val)
{
    io_cookie_t *io_cookie = cookie;
    uint32_t last_port = port + io_size - 1;
    cspacepath_t path;
    int error;
    error = vka_cspace_alloc_path(io_cookie->vka, &path);
    if (error) {
        ZF_LOGE("Failed to allocate slot");
        return error;
    }
    error = simple_get_IOPort_cap(io_cookie->simple, port, last_port, path.root, path.capPtr, path.capDepth);
    if (error) {
        ZF_LOGE("Failed to get capability for IOPort range 0x%x-0x%x", port, last_port);
        return -1;
    }

    int result;
    switch (io_size) {
    case 1:
        result = seL4_X86_IOPort_Out8(path.capPtr, port, val);
        break;
    case 2:
        result = seL4_X86_IOPort_Out16(path.capPtr, port, val);
        break;
    case 4:
        result = seL4_X86_IOPort_Out32(path.capPtr, port, val);
        break;
    default:
        ZF_LOGE("Invalid io_size %d, expected 1, 2 or 4", io_size);
        result = -1;
        break;
    }
    vka_cnode_delete(&path);
    vka_cspace_free_path(io_cookie->vka, path);
    return result;
}

int
sel4platsupport_get_io_port_ops(ps_io_port_ops_t *ops, simple_t *simple, vka_t *vka)
{
    assert(ops != NULL);
    assert(simple != NULL);
    assert(vka != NULL);
    io_cookie_t *cookie = malloc(sizeof(*cookie));
    if (!cookie) {
        return -1;
    }
    cookie->simple = simple;
    cookie->vka = vka;
    ops->io_port_in_fn = sel4platsupport_io_port_in;
    ops->io_port_out_fn = sel4platsupport_io_port_out;
    ops->cookie = (void *) cookie;

    return 0;
}

int sel4platsupport_new_arch_ops(ps_io_ops_t *ops, simple_t *simple, vka_t *vka)
{
    if (!ops || !simple) {
        return EINVAL;
    }

    return sel4platsupport_get_io_port_ops(&ops->io_port_ops, simple, vka);
}

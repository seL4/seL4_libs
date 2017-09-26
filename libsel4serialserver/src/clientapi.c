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
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <sel4/sel4.h>

#include <sel4utils/strerror.h>
#include <vka/vka.h>
#include <vka/capops.h>
#include <vka/object.h>
#include <vspace/vspace.h>

#include "serial_server.h"
#include <serial_server/client.h>

/** Single-call connection to the server thread, using the parent as a
 * proxy.
 *
 * Sends IPC to the parent thread, asking it to connect us to the server
 * thread. It does this by allocating some memory to be used as shmem, and then
 * sending a Frame cap to that memory, to the Parent, which then maps it into
 * the server.
 *
 * The Parent then replies by sending us an Endpoint cap which can be used to
 * communicate directly with the server thread from then on.
 */

int
serial_server_client_connect(seL4_CPtr badged_server_ep_cap,
                             vka_t *client_vka, vspace_t *client_vspace,
                             serial_client_context_t *conn)
{
    seL4_Error error;
    int shmem_n_pages;
    uintptr_t shmem_tmp_vaddr;
    seL4_MessageInfo_t tag;
    cspacepath_t frame_cspath;

    if (badged_server_ep_cap == 0 || client_vka == NULL || client_vspace == NULL
            || conn == NULL) {
        return seL4_InvalidArgument;
    }

    memset(conn, 0, sizeof(serial_client_context_t));

    shmem_n_pages = BYTES_TO_4K_PAGES(SERIAL_SERVER_SHMEM_MAX_SIZE);
    if (shmem_n_pages > seL4_MsgMaxExtraCaps) {
        ZF_LOGE(SERSERVC"connect: Currently unsupported shared memory size: "
                "IPC cap transfer capability is inadequate.");
        return seL4_RangeError;
    }
    conn->shmem = vspace_new_pages(client_vspace,
                                   seL4_AllRights,
                                   shmem_n_pages,
                                   seL4_PageBits);
    if (conn->shmem == NULL) {
        ZF_LOGE(SERSERVC"connect: Failed to alloc shmem.");
        return seL4_NotEnoughMemory;
    }
    assert(IS_ALIGNED((uintptr_t)conn->shmem, seL4_PageBits));

    /* Look up the Frame cap behind each page in the shmem range, and marshal
     * all of those Frame caps to the parent. The parent will then map those
     * Frames into its VSpace and establish a shmem link.
     */
    shmem_tmp_vaddr = (uintptr_t)conn->shmem;
    for (int i = 0; i < shmem_n_pages; i++) {
        vka_cspace_make_path(client_vka,
                             vspace_get_cap(client_vspace,
                                            (void *)shmem_tmp_vaddr),
                             &frame_cspath);

        seL4_SetCap(i, frame_cspath.capPtr);
        shmem_tmp_vaddr += BIT(seL4_PageBits);
    }

    /* Call the server asking it to establish the shmem mapping with us, and
     * get us connected up.
     */
    seL4_SetMR(SSMSGREG_FUNC, FUNC_CONNECT_REQ);
    seL4_SetMR(SSMSGREG_CONNECT_REQ_SHMEM_SIZE,
               SERIAL_SERVER_SHMEM_MAX_SIZE);
    /* extraCaps doubles up as the number of shmem pages. */
    tag = seL4_MessageInfo_new(0, 0,
                               shmem_n_pages,
                               SSMSGREG_CONNECT_REQ_END);

    tag = seL4_Call(badged_server_ep_cap, tag);

    /* It makes sense to verify that the message we're getting back is an
     * ACK response to our request message.
     */
    if (seL4_GetMR(SSMSGREG_FUNC) != FUNC_CONNECT_ACK) {
        error = seL4_IllegalOperation;
        ZF_LOGE(SERSERVC"connect: Reply message was not a CONNECT_ACK as "
                "expected.");
        goto out;
    }
    /* When the parent replies, we check to see if it was successful, etc. */
    error = seL4_MessageInfo_get_label(tag);
    if (error != (int)SERIAL_SERVER_NOERROR) {
        ZF_LOGE(SERSERVC"connect ERR %d: Failed to connect to the server.",
                error);

        if (error == (int)SERIAL_SERVER_ERROR_SHMEM_TOO_LARGE) {
            ZF_LOGE(SERSERVC"connect: Your requested shmem mapping size is too "
                    "large.\n\tServer's max shmem size is %luB.",
                    (long)seL4_GetMR(SSMSGREG_CONNECT_ACK_MAX_SHMEM_SIZE));
        }
        goto out;
    }

    conn->shmem_size = SERIAL_SERVER_SHMEM_MAX_SIZE;
    vka_cspace_make_path(client_vka, badged_server_ep_cap,
                         &conn->badged_server_ep_cspath);

    return seL4_NoError;

out:
    if (conn->shmem != NULL) {
        vspace_unmap_pages(client_vspace, (void *)conn->shmem, shmem_n_pages,
                           seL4_PageBits, VSPACE_FREE);
    }
    return error;
}

/** Performs the IPC register setup for a write() call to the server.
 *
 * The Server's ABI for the write() request has changed a little: the server
 * now returns the number of bytes it wrote out to the serial in a msg-reg,
 * aside from also returning an error code in the "label" of the header.
 *
 * @param conn Initialized connection token returned by
 *             serial_server_client_connect().
 * @param len length of the data in the buffer.
 * @param server_nbytes_written The value the server reports that it actually
 *                              wrote to the serial device.
 * @return 0 on success, or integer error value on error.
 */
static ssize_t
serial_server_write_ipc_invoke(serial_client_context_t *conn, ssize_t len)
{
    seL4_MessageInfo_t tag;

    seL4_SetMR(SSMSGREG_FUNC, FUNC_WRITE_REQ);
    seL4_SetMR(SSMSGREG_WRITE_REQ_BUFF_LEN, len);
    tag = seL4_MessageInfo_new(0, 0, 0, SSMSGREG_WRITE_REQ_END);

    tag = seL4_Call(conn->badged_server_ep_cspath.capPtr, tag);

    if (seL4_GetMR(SSMSGREG_FUNC) != FUNC_WRITE_ACK) {
        ZF_LOGE(SERSERVC"printf: Reply message was not a WRITE_ACK as "
                "expected.");
        return - seL4_IllegalOperation;
    }
    if (seL4_MessageInfo_get_label(tag) != 0) {
        return - seL4_MessageInfo_get_label(tag);
    }

    return seL4_GetMR(SSMSGREG_WRITE_ACK_N_BYTES_WRITTEN);
}

ssize_t
serial_server_printf(serial_client_context_t *conn, const char *fmt, ...)
{
    ssize_t expanded_fmt_length;
    va_list args;

    /* We simplify everything by just vsnprintf()ing, instead of marshaling
     * a variable length format string and a variable length argument list.
     */
    if (fmt == NULL || conn == NULL || conn->shmem == NULL) {
        ZF_LOGE(SERSERVC"printf: NULL passed for required arguments.\n"
                "\tIs connection handle valid?");
        return -seL4_InvalidArgument;
    }

    va_start(args, fmt);
    expanded_fmt_length = vsnprintf((char *)conn->shmem, conn->shmem_size,
                                    fmt, args);
    va_end(args);
    if (expanded_fmt_length < 0) {
        return -1;
    }

    if ((size_t)expanded_fmt_length >= conn->shmem_size) {
        ZF_LOGE(SERSERVC"printf: This printf call's total expanded length (%zd) "
                "exceeds your %zd bytes shmem buffer.\n\tMessage not sent to "
                "server.",
                expanded_fmt_length,
                conn->shmem_size);
        return -seL4_RangeError;
    }

    /* Else, send it off to the server. */
    return serial_server_write_ipc_invoke(conn, expanded_fmt_length);
}

ssize_t serial_server_flush(serial_client_context_t *conn, ssize_t len)
{
    if (len > conn->shmem_size) {
        return -seL4_RangeError;
    }

    if (len == 0) {
        return 0;
    }

    return serial_server_write_ipc_invoke(conn, len);
}

ssize_t
serial_server_write(serial_client_context_t *conn, const char *in_buff, ssize_t len)
{
    if (in_buff == NULL || conn == NULL || conn->shmem == NULL) {
        ZF_LOGE(SERSERVC"printf: NULL passed for required arguments.\n"
                "\tIs connection handle valid?");
        return -seL4_InvalidArgument;
    }
    if (len > conn->shmem_size) {
        return -seL4_RangeError;
    }

    if (len == 0) {
        return 0;
    }

    memcpy((void *)conn->shmem, in_buff, len);

    /* Else, send it off to the server. */
    return serial_server_write_ipc_invoke(conn, len);
}

void
serial_server_disconnect(serial_client_context_t *conn)
{
    seL4_MessageInfo_t tag;

    if (conn == NULL) {
        return;
    }

    seL4_SetMR(SSMSGREG_FUNC, FUNC_DISCONNECT_REQ);
    tag = seL4_MessageInfo_new(0, 0, 0, SSMSGREG_DISCONNECT_REQ_END);

    tag = seL4_Call(conn->badged_server_ep_cspath.capPtr, tag);

    if (seL4_GetMR(SSMSGREG_FUNC) != FUNC_DISCONNECT_ACK) {
        ZF_LOGE(SERSERVC"disconnect: reply message was not a DISCONNECT_ACK "
                "as expected.");
    }
}

int
serial_server_kill(serial_client_context_t *conn)
{
    seL4_MessageInfo_t tag;

    if (conn == NULL) {
        return seL4_InvalidArgument;
    }

    seL4_SetMR(SSMSGREG_FUNC, FUNC_KILL_REQ);
    tag = seL4_MessageInfo_new(0, 0, 0, SSMSGREG_KILL_REQ_END);

    tag = seL4_Call(conn->badged_server_ep_cspath.capPtr, tag);

    if (seL4_GetMR(SSMSGREG_FUNC) != FUNC_KILL_ACK) {
        ZF_LOGE(SERSERVC"kill: Reply message was not a KILL_ACK as expected.");
        return seL4_IllegalOperation;
    }
    return seL4_MessageInfo_get_label(tag);
}

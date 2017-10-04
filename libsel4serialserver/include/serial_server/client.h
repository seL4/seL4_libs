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
#pragma once

#include <sys/types.h>
#include <stdint.h>

#include <sel4/sel4.h>

#include <simple/simple.h>
#include <sel4utils/thread.h>
#include <vka/vka.h>
#include <vka/object.h>
#include <vspace/vspace.h>

/** @file API for making requests to a serial server multiplexing thread.
 *
 * If CONFIG_SERIAL_SERVER_COLOURED_OUTPUT is set, the output will also
 * be wrapped in ansi escape codes which allow client code to be identified
 * in a terminal.
 *
 * By default, the new thread which is created by this API will share the
 * CSpace and VSpace of its creator. Each client of the server thread is required
 * to have a badged cap to the Endpoint that the Server is listening on. The
 * badge on this Endpoint cap must have been allocated by the library's
 * serial_server_badge_value_alloc() or serial_server_badge_value_get_unused()
 * function.
 *
 * Establishing a connection to the server is as simple as calling:
 *  serial_client_context_t conn;
 *  error = serial_server_client_connect(..., &conn).
 * Then check the error value returned:
 *  if (error != SERIAL_SERVER_NOERROR) { ... }
 *
 * If the connect() call was successful, make sure you save the
 * serial_client_context_t object. Then use the serial server's printf from
 * that point on, by passing the serial_client_context_t object:
 *  serial_server_printf(&conn, "Hello world from %s!", "john");
 *
 * You can easily abstract away the long function name using preprocessor
 * defines or wrapper functions such as:
 *  #define printf(fmt, ...) serial_server_printf(&global_client_conn, ## __VA_ARGS__)
 *
 * CAUTION:
 * All vka_t, vpsace_t, and simple_t instances passed to this library by
 * reference must remain functional throughout the lifetime of the server.
 */

/* Context given to each client to preserve state.
 *
 * This is an opaque handle data type, and clients are not to assume the
 * layout or purpose of its members.
 */
typedef struct _serial_client_context {
    seL4_Word badge_value;
    cspacepath_t badged_server_ep_cspath;
    volatile char *shmem;
    size_t shmem_size;
} serial_client_context_t;

/** Establishes a connection to the server thread and returns a connection
 * handle. It is expected that this will be called by the client thread itself.
 *
 * CAUTION:
 * All vka_t, vpsace_t, and simple_t instances passed to this library by
 * reference must remain functional throughout the lifetime of the server.
 *
 * Can be called by any thread which has an endpoint to the thread/process
 * that spawned the server thread.
 * @param server_ep_cap CPtr to an endpoint between the client and the SERVER
 *                      thread.
 * @param client_vka Initialized vka_t for the client thread.
 * @param client_vspace Initialized vspace_t for the client thread.
 * @param client_badge_value Unique badge value for the connection between this
 *                           client and the server thread. Each client of the
 *                           server must have a unique badge value. Two clients
 *                           connecting with the same badge value will cause the
 *                           server to overwrite the former with the latter.
 * @param conn [out] Connection token returned by the library. Use this to make calls
 *             to the server thread.
 * @return Error value: 0 on success, non-zero on failure.
 */
int serial_server_client_connect(seL4_CPtr server_ep_cap,
                                 vka_t *client_vka,
                                 vspace_t *client_vspace,
                                 serial_client_context_t *conn);

/** Sends a request to the server to print a message to the serial.
 *
 * @param ctxt Valid connection token returned by serial_server_client_connect().
 * @param fmt Valid printf format specifier string.
 * @param ... Variadic argument list for printf.
 * @return The number of characters printed. If the call had to be aborted due
 *         to an error condition, a negative error code is returned.
 */
ssize_t serial_server_printf(serial_client_context_t *ctxt, const char *fmt, ...);


/** Sends the server a request to print the current contents of the shared memory buffer.
 *  For use when the client uses the shared memory buffer directly.
 *
 * @param ctxt Valid connection token returned by serial_server_client_connect().
 * @param len the size of the buffer data.
 * @return The number of bytes written (positive integer), or a negative integer
 *         for error condition.
 *
 */
ssize_t serial_server_flush(serial_client_context_t *ctxt, ssize_t len);

/** Sends a request to the server to write a fixed-length buffer to the serial.
 *
 * @param ctxt Valid connection token returned by serial_server_client_connect().
 * @param in_buff Input buffer of data.
 * @param len the size of the buffer data.
 * @return The number of bytes written (positive integer), or a negative integer
 *         for error condition.
 */
ssize_t serial_server_write(serial_client_context_t *ctxt, const char *in_buff, ssize_t len);

/** Sends a request to the server to disconnect the calling client.
 *
 * Causes the server to release the connection metadata it holds about the
 * client in question and teardown the shared memory mapping between the server
 * and that client.
 * @param ctxt Initialized connection token returned from
 *             serial_server_client_connect().
 */
void serial_server_disconnect(serial_client_context_t *ctxt);

/** Sends a request to the server to "kill" itself.
 *
 * In practice, right now that means that the server exits its message loop and
 * stops listening for IPC from clients, and then seL4_TCB_Suspend()s itself.
 * @param conn Connection handle to the server, returned by
 *             serial_server_client_connect().
 * @return Integer value: 0 on successfull "kill", non-zero if the server was
 *         unable to shutdown for some reason.
 */
int serial_server_kill(serial_client_context_t *conn);

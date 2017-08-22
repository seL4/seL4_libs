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

#include <stdint.h>

#include <sel4/sel4.h>

#include <simple/simple.h>
#include <sel4utils/thread.h>
#include <vka/vka.h>
#include <vka/object.h>
#include <vspace/vspace.h>

/** @file APIs for managing and interacting with the serial server thread.
 *
 * Defines the constants for the protocol, messages, and server-side state, as
 * well as the entry point and back-end routines for the server's API.
 *
 * All vka_t, vspace_t and simple_t instances that are supplied to this library
 * by the developer must persist and be functional for the lifetime of the
 * server thread.
 */

#define SERSERVS     "Serserv Server: "
#define SERSERVC     "Serserv Client: "
#define SERSERVP     "Serserv Parent: "

#define SERIAL_SERVER_BADGE_VALUE_EMPTY (0)

#define SERIAL_SERVER_SHMEM_MAX_SIZE (BIT(seL4_PageBits))

/* IPC values returned in the "label" message header. */
enum serial_server_errors {
    SERIAL_SERVER_NOERROR = 0,
    /* No future collisions with seL4_Error.*/
    SERIAL_SERVER_ERROR_SHMEM_TOO_LARGE = seL4_NumErrors,
    SERIAL_SERVER_ERROR_SERIAL_BIND_FAILED,
    SERIAL_SERVER_ERROR_UNKNOWN
};

/* IPC Message register values for SSMSGREG_FUNC */
enum serial_server_funcs {
    FUNC_CONNECT_REQ = 0,
    FUNC_CONNECT_ACK,

    FUNC_SERVER_SPAWN_SYNC_REQ,
    FUNC_SERVER_SPAWN_SYNC_ACK,

    FUNC_PRINTF_REQ,
    FUNC_PRINTF_ACK,

    FUNC_WRITE_REQ,
    FUNC_WRITE_ACK,

    FUNC_DISCONNECT_REQ,
    FUNC_DISCONNECT_ACK,

    FUNC_KILL_REQ,
    FUNC_KILL_ACK,
};

/* Designated purposes of each message register in the mini-protocol. */
enum serial_server_msgregs {
    /* These four are fixed headers in every serserv message. */
    SSMSGREG_FUNC = 0,
    /* This is a convenience label for IPC MessageInfo length. */
    SSMSGREG_LABEL0,

    SSMSGREG_CONNECT_REQ_SHMEM_SIZE = SSMSGREG_LABEL0,
    SSMSGREG_CONNECT_REQ_END,

    SSMSGREG_CONNECT_ACK_MAX_SHMEM_SIZE = SSMSGREG_LABEL0,
    SSMSGREG_CONNECT_ACK_END,

    SSMSGREG_SPAWN_SYNC_REQ_END = SSMSGREG_LABEL0,

    SSMSGREG_SPAWN_SYNC_ACK_END = SSMSGREG_LABEL0,

    SSMSGREG_PRINTF_REQ_FMT_STRLEN = SSMSGREG_LABEL0,
    SSMSGREG_PRINTF_REQ_END,

    SSMSGREG_PRINTF_ACK_PRINTF_RET = SSMSGREG_LABEL0,
    SSMSGREG_PRINTF_ACK_END,

    SSMSGREG_WRITE_REQ_BUFF_LEN = SSMSGREG_LABEL0,
    SSMSGREG_WRITE_REQ_END,

    SSMSGREG_WRITE_ACK_N_BYTES_WRITTEN = SSMSGREG_LABEL0,
    SSMSGREG_WRITE_ACK_END,

    SSMSGREG_DISCONNECT_REQ_END = SSMSGREG_LABEL0,

    SSMSGREG_DISCONNECT_ACK_END = SSMSGREG_LABEL0,

    SSMSGREG_KILL_REQ_END = SSMSGREG_LABEL0,

    SSMSGREG_KILL_ACK_END = SSMSGREG_LABEL0
};

/* Per-client context maintained by the server. */
typedef struct _serial_server_registry_entry {
    seL4_Word badge_value;
    volatile char *shmem;
    seL4_CPtr *shmem_frame_caps;
    size_t shmem_size;
} serial_server_registry_entry_t;

/* State maintained by the server. */
typedef struct _serial_server_context {
    simple_t *server_simple;
    vka_t *server_vka;
    seL4_CPtr server_cspace;
    cspacepath_t *frame_cap_recv_cspaths;
    vspace_t *server_vspace;
    sel4utils_thread_t server_thread;
    vka_object_t server_ep_obj;
    size_t shmem_max_size, shmem_max_n_pages;

    int registry_n_entries;
    serial_server_registry_entry_t *registry;

    seL4_Word parent_badge_value;
    cspacepath_t _badged_server_ep_cspath;
} serial_server_context_t;

/* Global server instance accessor functions. */
serial_server_context_t *get_serial_server(void);

/** Internal library function: acts as the main() for the server thread.
 */
void serial_server_main(void);

serial_server_registry_entry_t *serial_server_registry_get_entry_by_badge(seL4_Word badge_value);

/** Determines whether or not a badge value has been reserved and given out.
 * @param badge_value The badge value in question.
 * @return True only if the badge value has been given out.
 *         False if the badge value is invalid, or hasn't been given out.
 */
bool serial_server_badge_is_allocated(seL4_Word badge_value);

/** Returns an unused, unique badge value to the caller, and will NOT attempt
 * to resize the pool of available badge values to fulfill the request.
 *
 * The server maintains a list of badge values, so it can also be used to
 * allocate and ration out badge values.
 *
 * @return Returns a positive integer GREATER THAN 0 if successful.
 *         Returns 0 if unsuccessful.
 */
seL4_Word serial_server_badge_value_get_unused(void);

/** Returns a new, unique badge value to the caller, and WILL allocate new
 * badge values to satisfy the request.
 *
 * @return Returns a positive integer GREATER THAN 0 if successful.
 *         Returns 0 if unsuccessful.
 */
seL4_Word serial_server_badge_value_alloc(void);

/** Returns a badge value to the pool of available badge values.
 * @param badge_value The badge value to free.
 */
void serial_server_badge_value_free(seL4_Word badge_value);

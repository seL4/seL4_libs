/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <autoconf.h>
#include <sel4serialserver/gen_config.h>

#include <stdio.h>
#include <string.h>

#include <sel4/sel4.h>
#include <vka/vka.h>
#include <vka/object.h>
#include <vka/capops.h>

#include <utils/arith.h>
#include <utils/ansi.h>
#include <sel4utils/api.h>
#include <sel4utils/strerror.h>
#include <sel4platsupport/platsupport.h>

#include "serial_server.h"
#include <serial_server/client.h>
#include <serial_server/parent.h>

/* Define global instance. */
static serial_server_context_t serial_server;

static char *colors[] = {
    ANSI_COLOR(RED),
    ANSI_COLOR(GREEN),
    ANSI_COLOR(YELLOW),
    ANSI_COLOR(BLUE),
    ANSI_COLOR(MAGENTA),
    ANSI_COLOR(CYAN),

    ANSI_COLOR(RED, BOLD),
    ANSI_COLOR(GREEN, BOLD),
    ANSI_COLOR(YELLOW, BOLD),
    ANSI_COLOR(BLUE, BOLD),
    ANSI_COLOR(MAGENTA, BOLD),
    ANSI_COLOR(CYAN, BOLD),

    ANSI_COLOR(RED, ITALIC),
    ANSI_COLOR(GREEN, ITALIC),
    ANSI_COLOR(YELLOW, ITALIC),
    ANSI_COLOR(BLUE, ITALIC),
    ANSI_COLOR(MAGENTA, ITALIC),
    ANSI_COLOR(CYAN, ITALIC),

    ANSI_COLOR(RED, UNDERLINE),
    ANSI_COLOR(GREEN, UNDERLINE),
    ANSI_COLOR(YELLOW, UNDERLINE),
    ANSI_COLOR(BLUE, UNDERLINE),
    ANSI_COLOR(MAGENTA, UNDERLINE),
    ANSI_COLOR(CYAN, UNDERLINE),
};

#define NUM_COLORS ARRAY_SIZE(colors)
#define BADGE_TO_COLOR(badge) (colors[(badge) % NUM_COLORS])

serial_server_context_t *get_serial_server(void)
{
    return &serial_server;
}

static inline seL4_MessageInfo_t recv(seL4_Word *sender_badge)
{
    return api_recv(get_serial_server()->server_ep_obj.cptr, sender_badge, get_serial_server()->server_thread.reply.cptr);
}

static inline void reply(seL4_MessageInfo_t tag)
{
    api_reply(get_serial_server()->server_thread.reply.cptr, tag);
}

serial_server_registry_entry_t *serial_server_registry_get_entry_by_badge(seL4_Word badge_value)
{
    if (badge_value == SERIAL_SERVER_BADGE_VALUE_EMPTY
        || get_serial_server()->registry == NULL
        || badge_value > get_serial_server()->registry_n_entries) {
        return NULL;
    }
    /* If the badge value has been released, return NULL. */
    if (get_serial_server()->registry[badge_value - 1].badge_value
        == SERIAL_SERVER_BADGE_VALUE_EMPTY) {
        return NULL;
    }

    return &get_serial_server()->registry[badge_value - 1];
}

bool serial_server_badge_is_allocated(seL4_Word badge_value)
{
    serial_server_registry_entry_t *tmp;

    tmp = serial_server_registry_get_entry_by_badge(badge_value);
    if (tmp == NULL) {
        return false;
    }

    return tmp->badge_value != SERIAL_SERVER_BADGE_VALUE_EMPTY;
}

seL4_Word serial_server_badge_value_get_unused(void)
{
    if (get_serial_server()->registry == NULL) {
        return SERIAL_SERVER_BADGE_VALUE_EMPTY;
    }

    for (int i = 0; i < get_serial_server()->registry_n_entries; i++) {
        if (get_serial_server()->registry[i].badge_value != SERIAL_SERVER_BADGE_VALUE_EMPTY) {
            continue;
        }

        /* Badge value 0 will never be allocated, so index 0 is actually
         * badge 1, and index 1 is badge 2, and so on ad infinitum.
         */
        get_serial_server()->registry[i].badge_value = i + 1;
        return get_serial_server()->registry[i].badge_value;
    }

    return SERIAL_SERVER_BADGE_VALUE_EMPTY;
}

seL4_Word serial_server_badge_value_alloc(void)
{
    serial_server_registry_entry_t *tmp;
    seL4_Word ret;

    ret = serial_server_badge_value_get_unused();
    if (ret != SERIAL_SERVER_BADGE_VALUE_EMPTY) {
        /* Success */
        return ret;
    }

    tmp = realloc(get_serial_server()->registry,
                  sizeof(*get_serial_server()->registry)
                  * (get_serial_server()->registry_n_entries + 1));
    if (tmp == NULL) {
        ZF_LOGD(SERSERVS"badge_value_alloc: Failed resize pool.");
        return SERIAL_SERVER_BADGE_VALUE_EMPTY;
    }

    get_serial_server()->registry = tmp;
    get_serial_server()->registry[get_serial_server()->registry_n_entries].badge_value =
        SERIAL_SERVER_BADGE_VALUE_EMPTY;
    get_serial_server()->registry_n_entries++;

    /* If it fails again (some other caller raced us and got the new ID before
     * we did) that's tough luck -- the caller should probably look into
     * serializing the calls.
     */
    return serial_server_badge_value_get_unused();
}

void serial_server_badge_value_free(seL4_Word badge_value)
{
    serial_server_registry_entry_t *tmp;

    if (badge_value == SERIAL_SERVER_BADGE_VALUE_EMPTY) {
        return;
    }

    tmp = serial_server_registry_get_entry_by_badge(badge_value);
    if (tmp == NULL) {
        return;
    }

    tmp->badge_value = SERIAL_SERVER_BADGE_VALUE_EMPTY;
}

static void serial_server_registry_insert(seL4_Word badge_value, void *shmem,
                                          seL4_CPtr *shmem_frame_caps,
                                          size_t shmem_size)
{
    serial_server_registry_entry_t *tmp;

    tmp = serial_server_registry_get_entry_by_badge(badge_value);
    /* If this is NULL, something went very wrong, because this function is only
     * called after the server checks to ensure that the badge value has been
     * allocated in the registry. Technically, this should never happen, but
     * an assert doesn't hurt.
     */
    assert(tmp != NULL);

    tmp->badge_value = badge_value;
    tmp->shmem = shmem;
    tmp->shmem_size = shmem_size;
    tmp->shmem_frame_caps = shmem_frame_caps;
}

static void serial_server_registry_remove(seL4_Word badge_value)
{
    serial_server_registry_entry_t *tmp;

    tmp = serial_server_registry_get_entry_by_badge(badge_value);
    if (tmp == NULL) {
        return;
    }
    serial_server_badge_value_free(badge_value);
}

static void serial_server_set_frame_recv_path(void)
{
    seL4_SetCapReceivePath(get_serial_server()->server_cspace,
                           get_serial_server()->frame_cap_recv_cspaths[0].capPtr,
                           get_serial_server()->frame_cap_recv_cspaths[0].capDepth);
}

/** Processes all FUNC_CONNECT_REQ IPC messages. Establishes
 * shared mem mappings with new clients and sets up book-keeping metadata.
 *
 * Clients calling connect() will pass us a list of Frame caps which we must
 * map in order to establish shared mem with those clients. In this function,
 * the library maps the client's frames into the server's VSpace.
 */
seL4_Error serial_server_func_connect(seL4_MessageInfo_t tag,
                                      seL4_Word client_badge_value,
                                      size_t client_shmem_size)
{
    seL4_Error error;
    size_t client_shmem_n_pages;
    void *shmem_tmp = NULL;
    seL4_CPtr *client_frame_caps = NULL;
    cspacepath_t client_frame_cspath_tmp;

    if (client_shmem_size == 0) {
        ZF_LOGW(SERSERVS"connect: Invalid shared mem window size of 0B.\n");
        return seL4_InvalidArgument;
    }

    client_shmem_n_pages = BYTES_TO_4K_PAGES(client_shmem_size);
    /* The client should be allocated a badge value by the Parent, before it
     * attempts to connect to the Server.
     *
     * The reason being that when the badge is allocated, the metadata array
     * is resized as well, so badge allocation is also metadata allocation.
     */
    if (client_badge_value == SERIAL_SERVER_BADGE_VALUE_EMPTY
        || !serial_server_badge_is_allocated(client_badge_value)) {
        ZF_LOGW(SERSERVS"connect: Please allocate a badge value to this new "
                "client.\n");
        return -1;
    }
    /* Make sure that the client didn't request a shmem mapping larger than the
     * server is willing to handle.
     */
    if (client_shmem_n_pages > BYTES_TO_4K_PAGES(get_serial_server()->shmem_max_size)) {
        /* If the client asks for a shmem mapping too large, we refuse, and
         * send the value of shmem_max_size in SSMSGREG_RESPONSE
         * so it can try again.
         */
        ZF_LOGW(SERSERVS"connect: New client badge %x is asking to establish "
                "shmem mapping of %dB, but server max accepted shmem size is "
                "%dB.",
                client_badge_value, client_shmem_size,
                get_serial_server()->shmem_max_size);
        return (seL4_Error) SERIAL_SERVER_ERROR_SHMEM_TOO_LARGE;
    }

    if (seL4_MessageInfo_get_extraCaps(tag) != client_shmem_n_pages) {
        ZF_LOGW(SERSERVS"connect: Received %d Frame caps from client "
                "badge %x.\n\tbut client requested shmem mapping of %d "
                "frames. Possible cap transfer error.",
                seL4_MessageInfo_get_extraCaps(tag), client_badge_value,
                client_shmem_n_pages);
        return seL4_InvalidCapability;
    }

    /* Prepare an array of the client's shmem Frame caps to be mapped into our
     * VSpace.
     */
    client_frame_caps = calloc((client_shmem_n_pages + 1), sizeof(seL4_CPtr));
    if (client_frame_caps == NULL) {
        ZF_LOGE(SERSERVS"connect: Failed to alloc frame cap list for client "
                "shmem.");
        return seL4_NotEnoughMemory;
    }
    for (size_t i = 0; i < client_shmem_n_pages; i++) {
        /* For each frame, we need to make an seL4_CNode_Copy of it before
         * we zero-out the receive slots, or else when we delete the receive
         * slots, the frames will be revoked and unmapped.
         */
        error = vka_cspace_alloc_path(get_serial_server()->server_vka,
                                      &client_frame_cspath_tmp);
        if (error != 0) {
            ZF_LOGE(SERSERVS"connect: Failed to alloc CSpace slot for frame "
                    "%zd of %zd received from client badge %lx.",
                    i + 1, client_shmem_n_pages, (long)client_badge_value);
            goto out;
        }

        /* Copy the frame cap from the recv slot to the perm slot now. */
        error = vka_cnode_move(&client_frame_cspath_tmp,
                               &get_serial_server()->frame_cap_recv_cspaths[i]);
        if (error != 0) {
            ZF_LOGE(SERSERVS"connect: Failed to move %zuth frame-cap received "
                    " from client badge %lx.", i + 1, (long)client_badge_value);
            goto out;
        }

        client_frame_caps[i] = client_frame_cspath_tmp.capPtr;
        ZF_LOGD("connect: moved received client Frame cap %d from recv slot %"PRIxPTR" to slot %"PRIxPTR".",
                i + 1, get_serial_server()->frame_cap_recv_cspaths[i].capPtr,
                client_frame_caps[i]);
    }

    /* Map the frames into the vspace. */
    shmem_tmp = vspace_map_pages(get_serial_server()->server_vspace, client_frame_caps,
                                 NULL,
                                 seL4_AllRights, client_shmem_n_pages,
                                 seL4_PageBits,
                                 true);
    if (shmem_tmp == NULL) {
        ZF_LOGE(SERSERVS"connect: Failed to map shmem.");
        error = seL4_NotEnoughMemory;
        goto out;
    }

    serial_server_registry_insert(client_badge_value, shmem_tmp,
                                  client_frame_caps, client_shmem_size);

    ZF_LOGI(SERSERVS"connect: New client: badge %x, shmem %p, %d pages.",
            client_badge_value, shmem_tmp, client_shmem_n_pages);

    return seL4_NoError;

out:
    if (shmem_tmp != NULL) {
        vspace_unmap_pages(get_serial_server()->server_vspace, shmem_tmp,
                           client_shmem_n_pages, seL4_PageBits,
                           VSPACE_PRESERVE);
    }

    if (client_frame_caps != NULL) {
        for (size_t i = 0; i < client_shmem_n_pages; i++) {
            /* Because client_frame_caps was alloc'd with calloc, we can depend on
             * the unused slots being filled with 0s. Break on first unallocated.
             */
            if (client_frame_caps[i] == 0) {
                break;
            }
            client_frame_cspath_tmp.capPtr = client_frame_caps[i];
            vka_cspace_free_path(get_serial_server()->server_vka,
                                 client_frame_cspath_tmp);
        }
    }

    free(client_frame_caps);
    return error;
}

static int serial_server_func_write(serial_server_registry_entry_t *client_data,
                                    size_t message_len, size_t *bytes_written)
{
    *bytes_written = 0;

    if (client_data == NULL || bytes_written == NULL) {
        ZF_LOGE(SERSERVS"printf: Got NULL for required argument.");
        return seL4_InvalidArgument;
    }
    if (message_len > client_data->shmem_size) {
        return seL4_RangeError;
    }

    /* Write out */
    if (config_set(CONFIG_SERIAL_SERVER_COLOURED_OUTPUT)) {
        printf("%s", COLOR_RESET);
        printf("%s", BADGE_TO_COLOR(client_data->badge_value));
    }
    fwrite((void *)client_data->shmem, message_len, 1, stdout);
    if (config_set(CONFIG_SERIAL_SERVER_COLOURED_OUTPUT)) {
        printf("%s", COLOR_RESET);
    }
    *bytes_written = message_len;
    return 0;
}

static void serial_server_func_disconnect(serial_server_registry_entry_t *client_data)
{
    /* Tear down shmem and release the badge value for reuse. */
    vspace_unmap_pages(get_serial_server()->server_vspace,
                       (void *)client_data->shmem,
                       BYTES_TO_4K_PAGES(client_data->shmem_size),
                       seL4_PageBits, get_serial_server()->server_vka);
    free(client_data->shmem_frame_caps);
    serial_server_registry_remove(client_data->badge_value);
}

static void serial_server_func_kill(void)
{
    /* Tear down all existing connections. */
    for (int i = 0; i < get_serial_server()->registry_n_entries; i++) {
        serial_server_registry_entry_t *curr = &get_serial_server()->registry[i];

        if (curr->badge_value == SERIAL_SERVER_BADGE_VALUE_EMPTY
            || BYTES_TO_4K_PAGES(curr->shmem_size) == 0) {
            continue;
        }

        serial_server_func_disconnect(&get_serial_server()->registry[i]);
    }
}

/** Debugging function -- prints out the state of the registry and the server's
 * current list of clients.
 *
 * Can either print shallow or deep.
 * @param dump_frame_caps If true, print the array of frames caps that underlie
 *                        the shared mem mapping to each client.
 */
void serial_server_registry_dump(bool dump_frame_caps)
{
    serial_server_registry_entry_t *tmp;

    if (get_serial_server()->registry == NULL) {
        ZF_LOGD("Registry is NULL.");
    }
    for (int i = 0; i < get_serial_server()->registry_n_entries; i++) {
        tmp = &get_serial_server()->registry[i];
        ZF_LOGD("Reg: idx %d, badge %d, shmem_size %d, shmem %p, caps %p.",
                i, tmp->badge_value, tmp->shmem_size, tmp->shmem,
                tmp->shmem_frame_caps);

        if (!dump_frame_caps) {
            continue;
        }

        for (int j = 0; j < BYTES_TO_4K_PAGES(tmp->shmem_size); j++) {
            ZF_LOGD("Reg badge %d: frame cap %d: %"PRIxPTR".",
                    tmp->badge_value, j + 1, tmp->shmem_frame_caps[j]);
        }
    }
}

void serial_server_main(void)
{
    seL4_MessageInfo_t tag;
    seL4_Word sender_badge;
    enum serial_server_funcs func;
    int keep_going = 1;
    UNUSED seL4_Error error;
    serial_server_registry_entry_t *client_data = NULL;
    size_t buff_len, bytes_written;

    /* Bind to the serial driver. */
    error = platsupport_serial_setup_simple(get_serial_server()->server_vspace,
                                            get_serial_server()->server_simple,
                                            get_serial_server()->server_vka);
    if (error != 0) {
        ZF_LOGE(SERSERVS"main: Failed to bind to serial.");
    } else {
        ZF_LOGI(SERSERVS"main: Bound to the serial driver.");
    }

    /* The Parent will seL4_Call() the us, the Server, right after spawning us.
     * It will expect us to seL4_Reply() with an error status code - we will
     * send this Reply regardless of the outcome of the platform serial bind
     * operation.
     *
     * First call seL4_Recv() to get the Reply cap back to the Parent, and then
     * seL4_Reply to report our status.
     */
    recv(&sender_badge);

    seL4_SetMR(SSMSGREG_FUNC, FUNC_SERVER_SPAWN_SYNC_ACK);
    tag = seL4_MessageInfo_new(error, 0, 0, SSMSGREG_SPAWN_SYNC_ACK_END);
    reply(tag);

    /* If the bind failed, this thread has essentially failed its mandate, so
     * there is no reason to leave it scheduled. Kill it (to whatever extent
     * that is possible).
     */
    if (error != 0) {
        seL4_TCB_Suspend(get_serial_server()->server_thread.tcb.cptr);
    }

    ZF_LOGI(SERSERVS"main: Entering main loop and accepting requests.");
    while (keep_going) {
        /* Set the CNode slots where caps from clients will go */
        serial_server_set_frame_recv_path();

        tag = recv(&sender_badge);
        ZF_LOGD(SERSERVS "main: Got message from %x", sender_badge);

        func = seL4_GetMR(SSMSGREG_FUNC);

        /* Lookup the registry entry for this sender to make sure that the sender
         * has a shmem buffer registered with us. If not, ignore the message.
         * New connection requests are of course, exempt from the requirement to
         * already have an established connection.
         */
        if (func != FUNC_CONNECT_REQ) {
            client_data = serial_server_registry_get_entry_by_badge(sender_badge);
            if (client_data == NULL) {
                ZF_LOGW(SERSERVS"main: Got message from unregistered client "
                        "badge %x. Ignoring.",
                        sender_badge);
                continue;
            }
        }

        switch (func) {
        case FUNC_CONNECT_REQ:
            ZF_LOGD(SERSERVS"main: Got connect request from client badge %x.",
                    sender_badge);
            error = serial_server_func_connect(tag,
                                               sender_badge,
                                               seL4_GetMR(SSMSGREG_CONNECT_REQ_SHMEM_SIZE));

            seL4_SetMR(SSMSGREG_FUNC, FUNC_CONNECT_ACK);
            seL4_SetMR(SSMSGREG_CONNECT_ACK_MAX_SHMEM_SIZE,
                       get_serial_server()->shmem_max_size);
            tag = seL4_MessageInfo_new(error, 0, 0, SSMSGREG_CONNECT_ACK_END);
            reply(tag);
            break;

        case FUNC_WRITE_REQ:
            /* The Server's ABI for the write() request is as follows:
             *
             * The server returns the number of bytes it actually wrote out to the
             * serial in a msg-reg (SSMSGREG_WRITE_ACK_N_BYTES_WRITTEN),
             * aside from also returning an error code in the "label" of the header.
             *
             * This was done because attempting to overload the "label" of the
             * header to hold a value that can be either:
             *      (1) a negative integer error code, (2) a positive size_t
             *      byte length
             * Was not very clean especially since the bit-width of the "label"
             * field shouldn't be assumed lightly, so separating the error code
             * from the number of bytes written was the cleaner approach.
             *
             * At the client end, the client can decide to overload an ssize_t
             * to encode both types of values.
             */

            ZF_LOGD(SERSERVS"main: Got write request from client badge %x.",
                    sender_badge);
            buff_len = seL4_GetMR(SSMSGREG_WRITE_REQ_BUFF_LEN);
            error = serial_server_func_write(client_data, buff_len,
                                             &bytes_written);

            seL4_SetMR(SSMSGREG_FUNC, FUNC_WRITE_ACK);
            seL4_SetMR(SSMSGREG_WRITE_ACK_N_BYTES_WRITTEN, bytes_written);
            tag = seL4_MessageInfo_new(error, 0, 0, SSMSGREG_WRITE_ACK_END);
            reply(tag);
            break;

        case FUNC_DISCONNECT_REQ:
            ZF_LOGD(SERSERVS"main: Got disconnect request from client badge %x.",
                    sender_badge);
            serial_server_func_disconnect(client_data);

            seL4_SetMR(SSMSGREG_FUNC, FUNC_DISCONNECT_ACK);
            tag = seL4_MessageInfo_new(error, 0, 0, SSMSGREG_DISCONNECT_ACK_END);
            reply(tag);
            break;

        case FUNC_KILL_REQ:
            ZF_LOGI(SERSERVS"main: Got KILL request from client badge %x.",
                    sender_badge);
            /* The actual contents of the Reply don't matter here. */
            seL4_SetMR(SSMSGREG_FUNC, FUNC_KILL_ACK);
            tag = seL4_MessageInfo_new(0, 0, 0, SSMSGREG_KILL_ACK_END);
            reply(tag);
            /* Break out of the loop */
            keep_going = 0;
            break;

        default:
            ZF_LOGW(SERSERVS "main: Unknown function %d requested.", func);
            break;
        }
    }

    serial_server_func_kill();
    /* After we break out of the loop, seL4_TCB_Suspend ourselves */
    ZF_LOGI(SERSERVS"main: Suspending.");
    seL4_TCB_Suspend(get_serial_server()->server_thread.tcb.cptr);
}

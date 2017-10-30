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
#include <stdlib.h>
#include <stdbool.h>

#include <sel4/sel4.h>
#include <sel4utils/strerror.h>
#include <vka/vka.h>
#include <vka/object.h>
#include <vka/object_capops.h>

#include "serial_server.h"
#include <serial_server/parent.h>

seL4_Error
serial_server_parent_spawn_thread(simple_t *parent_simple, vka_t *parent_vka,
                                  vspace_t *parent_vspace,
                                  uint8_t priority)
{
    const size_t shmem_max_size = SERIAL_SERVER_SHMEM_MAX_SIZE;
    seL4_Error error;
    size_t shmem_max_n_pages;
    cspacepath_t parent_cspace_cspath;
    seL4_MessageInfo_t tag;

    if (parent_simple == NULL || parent_vka == NULL || parent_vspace == NULL) {
        return seL4_InvalidArgument;
    }

    memset(get_serial_server(), 0, sizeof(serial_server_context_t));

    /* Get a CPtr to the parent's root cnode. */
    shmem_max_n_pages = BYTES_TO_4K_PAGES(shmem_max_size);
    vka_cspace_make_path(parent_vka, 0, &parent_cspace_cspath);

    get_serial_server()->server_vka = parent_vka;
    get_serial_server()->server_vspace = parent_vspace;
    get_serial_server()->server_cspace = parent_cspace_cspath.root;
    get_serial_server()->server_simple = parent_simple;

    /* Allocate the Endpoint that the server will be listening on. */
    error = vka_alloc_endpoint(parent_vka, &get_serial_server()->server_ep_obj);
    if (error != 0) {
        ZF_LOGE(SERSERVP"spawn_thread: failed to alloc endpoint, err=%d.",
                error);
        return error;
    }

    /* And also allocate a badged copy of the Server's endpoint that the Parent
     * can use to send to the Server. This is used to allow the Server to report
     * back to the Parent on whether or not the Server successfully bound to a
     * platform serial driver.
     *
     * This badged endpoint will be reused by the library as the Parent's badged
     * Endpoint cap, if the Parent itself ever chooses to connect() to the
     * Server later on.
     */

    get_serial_server()->parent_badge_value = serial_server_badge_value_alloc();
    if (get_serial_server()->parent_badge_value == SERIAL_SERVER_BADGE_VALUE_EMPTY) {
        error = seL4_NotEnoughMemory;
        goto out;
    }

    error = vka_mint_object(parent_vka, &get_serial_server()->server_ep_obj,
                            &get_serial_server()->_badged_server_ep_cspath,
                            seL4_AllRights,
                            get_serial_server()->parent_badge_value);
    if (error != 0) {
        ZF_LOGE(SERSERVP"spawn_thread: Failed to mint badged Endpoint cap to "
                "server.\n"
                "\tParent cannot confirm Server thread successfully spawned.");
        goto out;
    }

    /* Allocate enough Cnode slots in our CSpace to enable us to receive
     * frame caps from our clients, sufficient to cover "shmem_max_size".
     * The problem here is that we're sort of forced to assume that we get
     * these slots contiguously. If they're not, we have a problem.
     *
     * If a client tries to send us too many frames, we respond with an error,
     * and indicate our shmem_max_size in the SSMSGREG_RESPONSE
     * message register.
     */
    get_serial_server()->frame_cap_recv_cspaths = calloc(shmem_max_n_pages,
                                                         sizeof(cspacepath_t));
    if (get_serial_server()->frame_cap_recv_cspaths == NULL) {
        error = seL4_NotEnoughMemory;
        goto out;
    }

    for (size_t i = 0; i < shmem_max_n_pages; i++) {
        error = vka_cspace_alloc_path(parent_vka,
                                      &get_serial_server()->frame_cap_recv_cspaths[i]);
        if (error != 0) {
            ZF_LOGE(SERSERVP"spawn_thread: Failed to alloc enough cnode slots "
                    "to receive shmem frame caps equal to %zd bytes.",
                    shmem_max_size);
            goto out;
        }
    }

    sel4utils_thread_config_t config = thread_config_default(parent_simple, parent_cspace_cspath.root,
                                                             seL4_NilData, get_serial_server()->server_ep_obj.cptr, priority);
    error = sel4utils_configure_thread_config(parent_vka, parent_vspace, parent_vspace,
                                              config, &get_serial_server()->server_thread);
    if (error != 0) {
        ZF_LOGE(SERSERVP"spawn_thread: sel4utils_configure_thread failed "
                "with %d.", error);
        goto out;
    }

    NAME_THREAD(get_serial_server()->server_thread.tcb.cptr, "serial server");
    error = sel4utils_start_thread(&get_serial_server()->server_thread,
                                   (sel4utils_thread_entry_fn)&serial_server_main,
                                   NULL, NULL, 1);
    if (error != 0) {
        ZF_LOGE(SERSERVP"spawn_thread: sel4utils_start_thread failed with "
                "%d.", error);
        goto out;
    }

    /* When the Server is spawned, it will reply to tell us whether or not it
     * successfully bound itself to the platform serial device. Block here
     * and wait for that reply.
     */
    seL4_SetMR(SSMSGREG_FUNC, FUNC_SERVER_SPAWN_SYNC_REQ);
    tag = seL4_MessageInfo_new(0, 0, 0, SSMSGREG_SPAWN_SYNC_REQ_END);
    tag = seL4_Call(get_serial_server()->_badged_server_ep_cspath.capPtr, tag);

    /* Did all go well with the server? */
    if (seL4_GetMR(SSMSGREG_FUNC) != FUNC_SERVER_SPAWN_SYNC_ACK) {
        ZF_LOGE(SERSERVP"spawn_thread: Server thread sync message after spawn "
                "was not a SYNC_ACK as expected.");
        error = seL4_InvalidArgument;
        goto out;
    }
    error = seL4_MessageInfo_get_label(tag);
    if (error != 0) {
        ZF_LOGE(SERSERVP"spawn_thread: Server thread failed to bind to the "
                "platform serial device.");
        goto out;
    }

    get_serial_server()->shmem_max_size = shmem_max_size;
    get_serial_server()->shmem_max_n_pages = shmem_max_n_pages;
    return 0;

out:
    if (get_serial_server()->frame_cap_recv_cspaths != NULL) {
        for (size_t i = 0; i < shmem_max_n_pages; i++) {
            /* Since the array was allocated with calloc(), it was zero'd out. So
             * those indexes that didn't get allocated will have NULL in them.
             * Break early on the first index that has NULL.
             */
            if (get_serial_server()->frame_cap_recv_cspaths[i].capPtr == 0) {
                break;
            }
            vka_cspace_free_path(parent_vka, get_serial_server()->frame_cap_recv_cspaths[i]);
        }
    }
    free(get_serial_server()->frame_cap_recv_cspaths);

    if (get_serial_server()->_badged_server_ep_cspath.capPtr != 0) {
        vka_cspace_free_path(parent_vka, get_serial_server()->_badged_server_ep_cspath);
    }
    if (get_serial_server()->parent_badge_value != SERIAL_SERVER_BADGE_VALUE_EMPTY) {
        serial_server_badge_value_free(get_serial_server()->parent_badge_value);
    }
    vka_free_object(parent_vka, &get_serial_server()->server_ep_obj);
    return error;
}

int
serial_server_parent_vka_mint_endpoint(vka_t *client_vka,
                                       cspacepath_t *badged_server_ep_cspath)
{
    if (client_vka == NULL || badged_server_ep_cspath == NULL) {
        return seL4_InvalidArgument;
    }

    seL4_Word new_badge_value = serial_server_badge_value_alloc();
    if (new_badge_value == SERIAL_SERVER_BADGE_VALUE_EMPTY) {
        return -1;
    }

    /* Mint the Endpoint to a new client. */
    return vka_mint_object_inter_cspace(get_serial_server()->server_vka,
                                        &get_serial_server()->server_ep_obj,
                                        client_vka,
                                        badged_server_ep_cspath,
                                        seL4_AllRights,
                                        new_badge_value);
}

static inline void
parent_ep_obj_to_cspath(cspacepath_t *result)
{
    if (result == NULL) {
        return;
    }
    vka_cspace_make_path(get_serial_server()->server_vka,
                         get_serial_server()->server_ep_obj.cptr,
                         result);
}

int
serial_server_allocate_client_badged_ep(cspacepath_t dest_slot)
{
    cspacepath_t server_ep_cspath;

    seL4_Word new_badge_value = serial_server_badge_value_alloc();
    if (new_badge_value == SERIAL_SERVER_BADGE_VALUE_EMPTY) {
        return -1;
    }

    parent_ep_obj_to_cspath(&server_ep_cspath);
    return vka_cnode_mint(&dest_slot, &server_ep_cspath, seL4_AllRights,
                          new_badge_value);
}

seL4_CPtr
serial_server_parent_mint_endpoint_to_process(sel4utils_process_t *p)
{
    if (p == NULL) {
        return 0;
    }

    cspacepath_t server_ep_cspath;
    seL4_Word new_badge_value = serial_server_badge_value_alloc();
    if (new_badge_value == SERIAL_SERVER_BADGE_VALUE_EMPTY) {
        return -1;
    }

    parent_ep_obj_to_cspath(&server_ep_cspath);
    return sel4utils_mint_cap_to_process(p, server_ep_cspath,
                                         seL4_AllRights,
                                         new_badge_value);
}

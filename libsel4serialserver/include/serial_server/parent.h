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
#include <vka/vka.h>
#include <vspace/vspace.h>
#include <sel4utils/process.h>

/** @file API for allowing a thread to act as the parent to a serial server
 * thread.
 *
 * Provides the APIs for spawning the server thread, allocating new badge
 * values to client threads, and minting the Server's Endpoint to Client
 * threads.
 */

/** Spawns the server thread. Server thread is spawned within the VSpace and
 *  CSpace of the thread that spawned it.
 *
 * CAUTION:
 * All vka_t, vpsace_t, and simple_t instances passed to this library by
 * reference must remain functional throughout the lifetime of the server.
 *
 * @param parent_simple Initialized simple_t for the parent process that is
 *                      spawning the server thread.
 * @param parent_vka Initialized vka_t for the parent process that is spawning
 *                   the server thread.
 * @param parent_vspace Initialized vspace_t for the parent process that is
 *                      spawning the server thread.
 * @param priority Server thread's priority.
 * @return seL4_Error value.
 */
seL4_Error serial_server_parent_spawn_thread(simple_t *parent_simple,
                                             vka_t *parent_vka,
                                             vspace_t *parent_vspace,
                                             uint8_t priority);

/** Mints a new, badged copy of the Server's Endpoint cap into the caller's
 * already-provided slot, dest_slot.
 *
 * @param dest_slot The caller-allocated slot into which the caller expects the
 *                  new badged Endpoint cap to be placed.
 * @return 0 if successful, non-zero on error.
 */
int serial_server_allocate_client_badged_ep(cspacepath_t dest_slot);

/** Mints a new, badged copy of the Server's Endpoint cap and returns it.
 *
 * @param client_vka Initialized vka_t instance.
 * @param new_client_badge_value The badge value to assign the new Endpoint cap.
 * @param badged_server_ep_cspath [out] resultant new badged Endpoint cap.
 * @return 0 if successful, non-zero on error.
 */
int serial_server_parent_vka_mint_endpoint(vka_t *client_vka,
                                           cspacepath_t *badged_server_ep_cspath);

/** Mints a new, badged copy of the Server's Endpoint cap into the specified
 * destination process' CSpace, and returns the slot it was minto into, as its
 * return value.
 *
 * @param p Destination process.
 * @param new_client_badge_value badge value to assign to the new Endpoint cap.
 * @return The cap-ptr to the slot that the cap was minted into.
 */
seL4_CPtr serial_server_parent_mint_endpoint_to_process(sel4utils_process_t *p);

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

#include <string.h>
#include <stdio.h>

#include <sel4/sel4.h>
#include <vka/capops.h>
#include <sel4utils/thread.h>
#include <serial_server/parent.h>
#include <serial_server/client.h>

#include <sel4test/test.h>
#include <sel4test/testutil.h>

#define SERSERV_TEST_PRIO_SERVER    (seL4_MaxPrio - 1)

static const char *test_str = "Hello, world!\n";

void get_serial_server_parent_tests()
{
}

static int
test_server_spawn(struct env *env)
{
    int        error;

    error = serial_server_parent_spawn_thread(&env->simple,
                                              &env->vka, &env->vspace,
                                              SERSERV_TEST_PRIO_SERVER);

    test_eq(error, 0);
    return sel4test_get_result();
}
DEFINE_TEST(SERSERV_PARENT_001, "Serial server spawn test", test_server_spawn, true)

static int
test_parent_connect(struct env *env)
{
    int error;
    serial_client_context_t conn;
    cspacepath_t badged_server_ep_cspath;

    error = serial_server_parent_spawn_thread(&env->simple,
                                              &env->vka, &env->vspace,
                                              SERSERV_TEST_PRIO_SERVER);

    test_eq(error, 0);

    error = serial_server_parent_vka_mint_endpoint(&env->vka, &badged_server_ep_cspath);
    test_eq(error, 0);

    error = serial_server_client_connect(badged_server_ep_cspath.capPtr,
                                         &env->vka, &env->vspace, &conn);
    test_eq(error, 0);

    return  sel4test_get_result();
}
DEFINE_TEST(SERSERV_PARENT_002, "Test connecting to the server from a parent thread", test_parent_connect, true)

static int
test_parent_printf(struct env *env)
{
    int error;
    serial_client_context_t conn;
    cspacepath_t badged_server_ep_cspath;

    error = serial_server_parent_spawn_thread(&env->simple,
                                              &env->vka, &env->vspace,
                                              SERSERV_TEST_PRIO_SERVER);

    test_eq(error, 0);

    error = serial_server_parent_vka_mint_endpoint(&env->vka, &badged_server_ep_cspath);
    test_eq(error, 0);

    error = serial_server_client_connect(badged_server_ep_cspath.capPtr,
                                         &env->vka, &env->vspace, &conn);
    test_eq(error, 0);

    error = serial_server_printf(&conn, test_str);
    test_eq(error, (int)strlen(test_str));
    return sel4test_get_result();
}
DEFINE_TEST(SERSERV_PARENT_003, "Printf() from a connected parent thread", test_parent_printf, true)

static int
test_parent_write(struct env *env)
{
    int error;
    serial_client_context_t conn;
    cspacepath_t badged_server_ep_cspath;

    error = serial_server_parent_spawn_thread(&env->simple,
                                              &env->vka, &env->vspace,
                                              SERSERV_TEST_PRIO_SERVER);

    test_eq(error, 0);

    error = serial_server_parent_vka_mint_endpoint(&env->vka, &badged_server_ep_cspath);
    test_eq(error, 0);

    error = serial_server_client_connect(badged_server_ep_cspath.capPtr,
                                         &env->vka, &env->vspace, &conn);
    test_eq(error, 0);

    error = serial_server_write(&conn, test_str, strlen(test_str));
    test_eq(error, (int)strlen(test_str));

    return sel4test_get_result();
}
DEFINE_TEST(SERSERV_PARENT_004, "Write() from a connected parent thread", test_parent_write, true)

static int
test_parent_disconnect_reconnect_write_and_printf(struct env *env)
{
    int error;
    serial_client_context_t conn;
    cspacepath_t badged_server_ep_cspath;

    error = serial_server_parent_spawn_thread(&env->simple,
                                              &env->vka, &env->vspace,
                                              SERSERV_TEST_PRIO_SERVER);

    test_eq(error, 0);

    error = serial_server_parent_vka_mint_endpoint(&env->vka, &badged_server_ep_cspath);
    test_eq(error, 0);

    error = serial_server_client_connect(badged_server_ep_cspath.capPtr,
                                         &env->vka, &env->vspace, &conn);
    test_eq(error, 0);

    error = serial_server_printf(&conn, test_str);
    test_eq(error, (int)strlen(test_str));
    error = serial_server_write(&conn, test_str, strlen(test_str));
    test_eq(error, (int)strlen(test_str));

    /* Disconnect then reconnect and attempt to print */
    serial_server_disconnect(&conn);

    /* Need to re-obtain new badge values, as the Server may not hand out the
     * same badge values the second time. Free the previous Endpoint cap.
     */
    vka_cnode_delete(&badged_server_ep_cspath);
    error = serial_server_parent_vka_mint_endpoint(&env->vka, &badged_server_ep_cspath);
    test_eq(error, 0);

    error = serial_server_client_connect(badged_server_ep_cspath.capPtr,
                                         &env->vka, &env->vspace, &conn);
    test_eq(error, 0);

    error = serial_server_write(&conn, test_str, strlen(test_str));
    test_eq(error, (int)strlen(test_str));
    error = serial_server_printf(&conn, test_str);
    test_eq(error, (int)strlen(test_str));

    return sel4test_get_result();
}
DEFINE_TEST(SERSERV_PARENT_005,
            "Test printf() and write() from a parent thread when after a "
            "connection reset (disconnect/reconnect)",
            test_parent_disconnect_reconnect_write_and_printf,
            true)

static int
test_kill_from_parent(struct env *env)
{
    int error;
    serial_client_context_t conn;
    cspacepath_t badged_server_ep_cspath;

    error = serial_server_parent_spawn_thread(&env->simple,
                                              &env->vka, &env->vspace,
                                              SERSERV_TEST_PRIO_SERVER);
    test_eq(error, 0);

    error = serial_server_parent_vka_mint_endpoint(&env->vka, &badged_server_ep_cspath);
    test_eq(error, 0);

    error = serial_server_client_connect(badged_server_ep_cspath.capPtr,
                                         &env->vka, &env->vspace, &conn);
    test_eq(error, 0);

    /* Kill the Server from the parent. */
    error = serial_server_kill(&conn);
    test_eq(error, 0);

    return sel4test_get_result();
}
DEFINE_TEST(SERSERV_PARENT_006, "Kill the Server from the Parent thread",
            test_kill_from_parent, true)

static int
test_spawn_thread_inputs(struct env *env)
{
    int error;

    /* Test NULL inputs to spawn_thread(). */
    error = serial_server_parent_spawn_thread(NULL, &env->vka,
                                              &env->vspace,
                                              SERSERV_TEST_PRIO_SERVER);
    test_neq(error, 0);
    error = serial_server_parent_spawn_thread(&env->simple, NULL,
                                              &env->vspace,
                                              SERSERV_TEST_PRIO_SERVER);
    test_neq(error, 0);
    error = serial_server_parent_spawn_thread(&env->simple, &env->vka,
                                              NULL,
                                              SERSERV_TEST_PRIO_SERVER);
    test_neq(error, 0);

    return sel4test_get_result();
}
DEFINE_TEST(SERSERV_PARENT_007, "Test a series of unexpected input values to spawn_thread",
            test_spawn_thread_inputs, true)

static int
test_connect_inputs(struct env *env)
{
    int error;
    serial_client_context_t conn;
    cspacepath_t badged_server_ep_cspath;

    /* Test NULL inputs to connect(). */
    error = serial_server_parent_spawn_thread(&env->simple, &env->vka,
                                              &env->vspace,
                                              SERSERV_TEST_PRIO_SERVER);
    test_eq(error, 0);
    error = serial_server_parent_vka_mint_endpoint(NULL, &badged_server_ep_cspath);
    test_neq(error, 0);
    error = serial_server_parent_vka_mint_endpoint(&env->vka, NULL);
    test_neq(error, 0);
    error = serial_server_parent_vka_mint_endpoint(&env->vka, &badged_server_ep_cspath);
    test_eq(error, 0);

    error = serial_server_client_connect(0,
                                         &env->vka, &env->vspace, &conn);
    test_neq(error, 0);
    error = serial_server_client_connect(badged_server_ep_cspath.capPtr,
                                         NULL, &env->vspace, &conn);
    test_neq(error, 0);
    error = serial_server_client_connect(badged_server_ep_cspath.capPtr,
                                         &env->vka, NULL, &conn);
    test_neq(error, 0);
    error = serial_server_client_connect(badged_server_ep_cspath.capPtr,
                                         &env->vka, &env->vspace, NULL);
    test_neq(error, 0);

    return sel4test_get_result();
}
DEFINE_TEST(SERSERV_PARENT_008, "Test a series of unexpected input values to connect()",
            test_connect_inputs, true)

static int
test_printf_inputs(struct env *env)
{
    int error;
    serial_client_context_t conn;
    cspacepath_t badged_server_ep_cspath;

    /* Test NULL inputs to printf(). */
    error = serial_server_parent_spawn_thread(&env->simple, &env->vka,
                                              &env->vspace,
                                              SERSERV_TEST_PRIO_SERVER);
    test_eq(error, 0);
    error = serial_server_parent_vka_mint_endpoint(&env->vka, &badged_server_ep_cspath);
    test_eq(error, 0);
    error = serial_server_client_connect(badged_server_ep_cspath.capPtr,
                                         &env->vka, &env->vspace, &conn);
    test_eq(error, 0);

    error = serial_server_printf(NULL, test_str);
    test_neq(error, (int)strlen(test_str));
    error = serial_server_printf(&conn, NULL);
    test_neq(error, (int)strlen(test_str));

    return sel4test_get_result();
}
DEFINE_TEST(SERSERV_PARENT_009, "Test a series of unexpected input values to printf()",
            test_printf_inputs, true)

static int
test_write_inputs(struct env *env)
{
    int error;
    serial_client_context_t conn;
    cspacepath_t badged_server_ep_cspath;

    /* Test NULL inputs to printf(). */
    error = serial_server_parent_spawn_thread(&env->simple, &env->vka,
                                              &env->vspace,
                                              SERSERV_TEST_PRIO_SERVER);
    test_eq(error, 0);
    error = serial_server_parent_vka_mint_endpoint(&env->vka, &badged_server_ep_cspath);
    test_eq(error, 0);
    error = serial_server_client_connect(badged_server_ep_cspath.capPtr,
                                         &env->vka, &env->vspace, &conn);
    test_eq(error, 0);

    error = serial_server_write(NULL, test_str, strlen(test_str));
    test_neq(error, (int)strlen(test_str));
    error = serial_server_write(&conn, NULL, 500);
    test_neq(error, (int)strlen(test_str));
    error = serial_server_write(&conn, test_str, 0);
    test_neq(error, (int)strlen(test_str));

    return sel4test_get_result();
}
DEFINE_TEST(SERSERV_PARENT_010, "Test a series of unexpected input values to write()",
            test_write_inputs, true)


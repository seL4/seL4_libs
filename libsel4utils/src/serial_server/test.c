#include <string.h>
#include <stdio.h>

#include <sel4/sel4.h>
#include <vka/capops.h>
#include <sel4utils/thread.h>
#include <sel4utils/serial_server/parent.h>
#include <sel4utils/serial_server/client.h>

#include <sel4test/test.h>
#include <sel4test/testutil.h>
#define SERSERV_TEST_PRIO_SERVER    (seL4_MaxPrio - 1)
// TODO struct env

#define MAX_REGIONS 4
struct env {
    /* An initialised vka that may be used by the test. */
    vka_t vka;
    /* virtual memory management interface */
    vspace_t vspace;
    /* initialised timer */
    seL4_timer_t timer;
    /* abstract interface over application init */
    simple_t simple;
    /* notification for timer */
    vka_object_t timer_notification;

    /* caps for the current process */
    seL4_CPtr cspace_root;
    seL4_CPtr page_directory;
    seL4_CPtr endpoint;
    seL4_CPtr tcb;
    seL4_CPtr timer_untyped;
    seL4_CPtr asid_pool;
    seL4_CPtr asid_ctrl;
    seL4_CPtr sched_ctrl;
#ifdef CONFIG_IOMMU
    seL4_CPtr io_space;
#endif /* CONFIG_IOMMU */
#ifdef CONFIG_ARM_SMMU
    seL4_SlotRegion io_space_caps;
#endif
    seL4_Word cores;
    seL4_CPtr domain;

    int priority;
    int cspace_size_bits;
    int num_regions;
    sel4utils_elf_region_t regions[MAX_REGIONS];
};

static const char *test_str = "Hello, world!\n";

void get_serial_server_parent_tests() {
    printf("get serial server parent tests\n");
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
DEFINE_TEST(SERSERV_PARENT_001, "Serial server spawn test", test_server_spawn)

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
DEFINE_TEST(SERSERV_PARENT_002, "Test connecting to the server from a parent thread", test_parent_connect)

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
DEFINE_TEST(SERSERV_PARENT_003, "Printf() from a connected parent thread", test_parent_printf)

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
DEFINE_TEST(SERSERV_PARENT_004, "Write() from a connected parent thread", test_parent_write)

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
            test_parent_disconnect_reconnect_write_and_printf)

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
            test_kill_from_parent)

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
            test_spawn_thread_inputs)

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
            test_connect_inputs)

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
            test_printf_inputs)

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
            test_write_inputs)


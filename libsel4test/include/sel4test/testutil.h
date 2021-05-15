/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

/* Include Kconfig variables. */
#include <autoconf.h>
#include <sel4test/gen_config.h>
#include <stdbool.h>

typedef enum test_result {
    /* test passed */
    SUCCESS,
    /* test failed */
    FAILURE,
    /* test corrupted environment, abort tests */
    ABORT,
    /* Start of the free slot/ID to be used to request
     * operations from sel4test-test to sel4test-driver
     * using the same end point
     */
    SEL4TEST_RESULT_FREE
} test_result_t;

/* Communication codes/requests between a server and a client */
typedef enum _sel4test_communication_codes {
    SEL4TEST_TIME_MIN = SEL4TEST_RESULT_FREE,
    /* Client: requests a timeout from the server
     * Server: Notify clients after the elapsed requested time.
     * Previous timeout requests will be overwritten (if ongoing) with
     * new timeout requests.
     */
    SEL4TEST_TIME_TIMEOUT,
    /* Client: requests a timer reset and cancel previous requests
     * Server: Cleans up the resources used from previous requests
     * and stop notifying clients.
     */
    SEL4TEST_TIME_RESET,
    SEL4TEST_TIME_TIMESTAMP,

    SEL4TEST_TIME_MAX,

    SEL4TEST_PROTOBUF_RPC = SEL4TEST_TIME_MAX,

    SEL4TEST_OUTPUT_MAX
} sel4test_output_t;

/* A buffered printf to avoid corrupting xml output */
void sel4test_printf(const char *out);
/* enable printf buffering */
void sel4test_start_printf_buffer(void);
/* dump the current buffer and disable printf buffering */
void sel4test_end_printf_buffer(void);

/* reset the test environment for the next test */
void sel4test_reset(void);

/**
 * Report an error in a test case.
 * Can report multiple errors.
 * This will fail a test case.
 */
void _sel4test_report_error(const char *error, const char *file, int line);

/*
 * Mark the current test as failed. Should
 * only be called once per test case
 */
void _sel4test_failure(const char *failure, const char *file, int line);

/*
 * Mark the current test as fatally failed. The test will be terminated and
 * will not proceed beyond this point.
 */
void _sel4test_abort(const char *failure, const char *file, int line);

/*
 * Indicates if current test passed.
 */
test_result_t sel4test_get_result(void);

static inline bool sel4test_isTimerRPC(int output)
{
    return (output > SEL4TEST_TIME_MIN && output < SEL4TEST_TIME_MAX);
}

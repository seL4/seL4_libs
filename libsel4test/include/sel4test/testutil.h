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

/* Include Kconfig variables. */
#include <autoconf.h>
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

typedef enum _sel4test_communication_codes {
    SEL4TEST_TIME_SLEEP = SEL4TEST_RESULT_FREE,
    SEL4TEST_TIME_PERIODIC_START,
    SEL4TEST_TIME_PERIODIC_STOP,
    SEL4TEST_TIME_TIMESTAMP
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



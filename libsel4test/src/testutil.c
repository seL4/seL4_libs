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

#include <autoconf.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sel4test/test.h>
#include <sel4test/testutil.h>

#include <utils/util.h>

/* buffer for test stdout size */
#define STDOUT_CACHE 50000


/* buffer stdout here during a test if we're outputting xml */
static char current_stdout_bank[STDOUT_CACHE];
/* index we are up to in the buffer */
static int buf_index = 0;
/* is the buffer currently enabled? otherwise just printf */
static bool do_buffer_printf = false;

/* global to track if the current test has passed */
static test_result_t current_test_result = SUCCESS;

#undef printf
void
sel4test_printf(const char *string)
{
    if (!do_buffer_printf) {
        printf("%s", string);
    } else if (buf_index < STDOUT_CACHE && buf_index >= 0) {
        size_t len = STDOUT_CACHE - buf_index;
        snprintf(current_stdout_bank + buf_index, len, "%s", string);
        /* NULL terminate the destination in case 'string' was too long. */
        current_stdout_bank[STDOUT_CACHE - 1] = '\0';
        buf_index += strlen(string);
    }
}

void sel4test_start_printf_buffer(void)
{
    /* only enable the printf buffer if we are buffering output */
    if (config_set(CONFIG_PRINT_XML)) {
        buf_index = 0;
        do_buffer_printf = true;
    }
}

void sel4test_end_printf_buffer(void)
{
    do_buffer_printf = false;
    if (buf_index != 0) {
        printf("\t\t<system-out>%s</system-out>\n", current_stdout_bank);
        buf_index = 0;
    }

}

void _sel4test_report_error(const char *error, const char *file, int line)
{
    if (config_set(CONFIG_PRINT_XML)) {
        printf("\t\t<error>%s at line %d of file %s</error>\n", error, line, file);
    } else {
        printf("\tError: %s at line %d of file %s\n", error, line, file);
    }
    current_test_result = FAILURE;
}

void _sel4test_failure(const char *failure, const char *file, int line)
{
    if (config_set(CONFIG_PRINT_XML)) {
        printf("\t\t<failure type=\"failure\">%s at line %d of file %s</failure>\n", failure, line, file);
    } else {
        printf("\tFailure: %s at line %d of file %s\n", failure, line, file);
    }
    current_test_result = FAILURE;
}

void _sel4test_abort(const char *failure, const char *file, int line)
{
    if (config_set(CONFIG_PRINT_XML)) {
        printf("\t\t<failure type=\"failure\">%s at line %d of file %s</failure>\n", failure, line, file);
    } else {
        printf("\tFailure: %s at line %d of file %s\n", failure, line, file);
    }
    current_test_result = ABORT;
}

void sel4test_reset(void)
{
    current_test_result = SUCCESS;

    if (config_set(CONFIG_ENABLE_BENCHMARKS)) {
        seL4_BenchmarkResetLog();
    }
}

test_result_t sel4test_get_result(void)
{
    return current_test_result;
}

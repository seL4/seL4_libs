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

#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sel4test/test.h>
#include <sel4test/testutil.h>

#include <utils/util.h>

#define MAX_NAME_SIZE 100
#define STDOUT_CACHE 50000

#define BUFFERING_DISABLED (-1)

#ifdef CONFIG_BUFFER_OUTPUT
static char current_stdout_bank[STDOUT_CACHE];
static int buf_index = BUFFERING_DISABLED;
#endif

static bool current_test_passed = true;
static bool current_test_aborted = false;

#ifdef CONFIG_BUFFER_OUTPUT
#undef printf
void
sel4test_printf(const char *string)
{
    if (buf_index == BUFFERING_DISABLED) {
        printf("%s", string);
    } else if (buf_index < STDOUT_CACHE && buf_index >= 0) {
        size_t len = STDOUT_CACHE - buf_index;
        snprintf(current_stdout_bank + buf_index, len, "%s", string);
        /* NULL terminate the destination in case 'string' was too long. */
        current_stdout_bank[STDOUT_CACHE - 1] = '\0';
        buf_index += strlen(string);
    }
}

void sel4test_reset_buffer_index(void)
{
    buf_index = 0;
}

void sel4test_disable_buffering(void)
{
    buf_index = BUFFERING_DISABLED;
}

void sel4test_clear_buffer(void)
{
    memset(current_stdout_bank, 0, STDOUT_CACHE);
}

void sel4test_print_buffer(void)
{
    if (buf_index != 0) {
        printf("\t\t<system-out>%s</system-out>\n", current_stdout_bank);
        buf_index = 0;
    }
}
#endif /* CONFIG_BUFFER_OUTPUT */

void
sel4test_start_new_test(void)
{
    current_test_passed = true;
    current_test_aborted = false;
}

void
_sel4test_report_error(const char *error, const char *file, int line)
{
#ifdef CONFIG_PRINT_XML
    printf("\t\t<error>%s at line %d of file %s</error>\n", error, line, file);
#else
    printf("\tError: %s at line %d of file %s\n", error, line, file);
#endif /* CONFIG_PRINT_XML */
    current_test_passed = false;
}

void
_sel4test_failure(const char *failure, const char *file, int line)
{
#ifdef CONFIG_PRINT_XML
    printf("\t\t<failure type=\"failure\">%s at line %d of file %s</failure>\n", failure, line, file);
#else
    printf("\tFailure: %s at line %d of file %s\n", failure, line, file);
#endif /* CONFIG_PRINT_XML */
    current_test_passed = false;
}

void
_sel4test_abort(const char *failure, const char *file, int line)
{
    current_test_aborted = true;
    _sel4test_failure(failure, file, line);
}

bool
is_aborted()
{
    return current_test_aborted;
}

int
sel4test_get_result(void)
{
    return current_test_passed;
}


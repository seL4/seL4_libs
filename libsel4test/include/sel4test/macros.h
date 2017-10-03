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

#define SEL4TEST_PRINT_BUFFER 200

/**
 * When running a test suite, we want to output any output from the suite in a
 * <stdout></stdout> section. But for debugging we do not want this. Therefore
 * you should *disable* CONFIG_PRINT_XML while debugging tests.
 */
#ifdef CONFIG_PRINT_XML
#include <stdio.h>
#define printf(x, ...)  do {\
    char buffer[SEL4TEST_PRINT_BUFFER];\
    snprintf(buffer, SEL4TEST_PRINT_BUFFER, x, ##__VA_ARGS__);\
    sel4test_printf(buffer);\
} while(0)
#endif /* CONFIG_PRINT_XML */

#define sel4test_case_with_message(condition, message) do {\
    if(!(condition)) {\
        sel4test_report_error(message);\
    }\
} while(0)
/**
 * Like an assert but does not crash the build.
 */
#define sel4test_case(condition) do {\
    if(!(condition)) {\
        sel4test_report_error("");\
    }\
} while(0)

/**
 * Report an error in the test
 */
#define sel4test_report_error(x) _sel4test_report_error(x, __FILE__, __LINE__)

/**
 * Report a failure in the test
 */
#define sel4test_failure(x) _sel4test_failure(x, __FILE__, __LINE__)

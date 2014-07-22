/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef SEL4_TEST_PROTOTYPE_H
#define SEL4_TEST_PROTOTYPE_H

/* Include Kconfig variables. */
#include <autoconf.h>

#ifdef CONFIG_BUFFER_OUTPUT
void sel4test_printf(const char *out);
#endif /* CONFIG_BUFFER_OUTPUT */

/**
 * Start a test suite
 * 
 * @name name of test suite
 */
void sel4test_start_suite(char *name);


/**
 * End test suite
 */
void sel4test_end_suite(void);

/**
 * Start a test case.
 *
 * @param name the name of the test
 */
void sel4test_start_test(const char *name);


/**
 * Report an error in a test case.
 * Can report multiple errors.
 * This will fail a test case.
 */
void _sel4test_report_error(char *error, char *file, int line);


/*
 * Mark the current test as failed. Should
 * only be called once per test case 
 */
void _sel4test_failure(char *failure, char *file, int line);

/*
 * End the current test case
 */
void sel4test_end_test(void);

/**
 * Gets the results of current suite. Results of current 
 * suite is valid until next sel4test_start_suite call.
 *
 * @param out_num_tests number of tests (may be NULL)
 * @param out_passed how many tests passed (may be NULL)
 * @param out_failed how many tests failed (may be NULL)
 */
void sel4_test_get_suite_results(int *out_num_tests, int *out_passed, int *out_failed);

#endif /* SEL4_TEST_H */

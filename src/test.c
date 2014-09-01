/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <autoconf.h>

#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sel4test/prototype.h>
#include <sel4test/test.h>

#define MAX_NAME_SIZE 100
#define STDOUT_CACHE 50000

#define BUFFERING_DISABLED (-1)

#ifdef CONFIG_BUFFER_OUTPUT
static char sel4test_name[MAX_NAME_SIZE];
static char current_stdout_bank[STDOUT_CACHE];
static int buf_index = BUFFERING_DISABLED;
#endif

static int num_tests = 0;
static int num_tests_passed = 0;
static int current_test_passed = 1;

#ifdef CONFIG_BUFFER_OUTPUT
#undef printf
void 
sel4test_printf(const char *string) {
    if(buf_index == BUFFERING_DISABLED) {
        printf("%s", string);
    } else if (buf_index < STDOUT_CACHE && buf_index >= 0) {
        size_t len = STDOUT_CACHE - buf_index;
        snprintf(current_stdout_bank + buf_index, len, "%s", string);
        /* NULL terminate the destination in case 'string' was too long. */
        current_stdout_bank[STDOUT_CACHE - 1] = '\0';
        buf_index += strlen(string);
    }
}
#endif /* CONFIG_BUFFER_OUTPUT */


void 
sel4test_start_suite(char *name) {

#ifdef CONFIG_PRINT_XML
    buf_index = 0;
    printf("<testsuite>\n");
    strncpy(sel4test_name, name, MAX_NAME_SIZE);
    /* NULL terminate sel4test_name in case name was too long. */
    sel4test_name[MAX_NAME_SIZE - 1] = '\0';
#else
    printf("Starting test suite %s\n", name);
#endif /* CONFIG_PRINT_XML */

    num_tests = 0;
    num_tests_passed = 0;
    current_test_passed = 1;
}

void 
sel4test_end_suite(void) {
#ifdef CONFIG_PRINT_XML
    printf("</testsuite>\n"); 
    buf_index = BUFFERING_DISABLED;
#else
    if (num_tests_passed != num_tests) {
        printf("Test suite failed. %d/%d tests passed.\n", num_tests_passed, num_tests);
    } else {
        printf("Test suite passed. %d tests passed.\n", num_tests);
    }
#endif /* CONFIG_PRINT_XML */
}

void 
sel4test_start_test(const char *name) {
#ifdef CONFIG_BUFFER_OUTPUT
    buf_index = 0;
    memset(current_stdout_bank, 0, STDOUT_CACHE); 
#endif /* CONFIG_BUFFER_OUTPUT */
#ifdef CONFIG_PRINT_XML
    printf("\t<testcase classname=\"%s\" name=\"%s\">\n", sel4test_name, name);
#else
    printf("Starting test %d: %s\n", num_tests, name);
#endif /* CONFIG_PRINT_XML */
    num_tests++;
    current_test_passed = 1;
}

void 
_sel4test_report_error(char *error, char *file, int line) {
#ifdef CONFIG_PRINT_XML
    printf("\t\t<error>%s at line %d of file %s</error>\n", error, line, file);
#else
    printf("\tError: %s at line %d of file %s\n", error, line, file);    
#endif /* CONFIG_PRINT_XML */
    current_test_passed = 0;
}


void 
_sel4test_failure(char *failure, char *file, int line) {
#ifdef CONFIG_PRINT_XML
    printf("\t\t<failure type=\"failure\">%s at line %d of file %s</failure>\n", failure, line, file);
#else
    printf("\tFailure: %s at line %d of file %s\n", failure, line, file);
#endif /* CONFIG_PRINT_XML */
    current_test_passed = 0;
}

void 
sel4test_end_test(void) {
    if (current_test_passed) {
        num_tests_passed++;
    } else {
        _sel4test_failure("Test check failure (see log).", "", 0);
    }
#ifdef CONFIG_BUFFER_OUTPUT
    if (buf_index != 0) {
        printf("\t\t<system-out>%s</system-out>\n", current_stdout_bank);
        buf_index = 0;
    }
#endif /* CONFIG_BUFFER_OUTPUT */
#ifdef CONFIG_PRINT_XML
    printf("\t</testcase>\n");
#endif /* CONFIG_PRINT_XML */
}

void sel4_test_get_suite_results(int *out_num_tests, int *out_passed, int *out_failed) {
    if (out_num_tests) (*out_num_tests) = num_tests;
    if (out_passed) (*out_passed) = num_tests_passed;
    if (out_failed) (*out_failed) = num_tests - num_tests_passed;
}

/* Definitions so that we can find the test cases */
extern struct testcase __start__test_case[];
extern struct testcase __stop__test_case[];


testcase_t*
sel4test_get_test(char *name) 
{
    
    for (testcase_t *t = __start__test_case; t < __stop__test_case; t++) {
        if (strcmp(name, t->name) == 0) {
            return t;
        }
    }

    return NULL;
}


void
sel4test_run_tests(char *name, int (*run_test)(struct testcase *t)) {

    /* Count how many tests actually exist and allocate space for them */
    int max_tests = (int)(__stop__test_case - __start__test_case);
    struct testcase *tests[max_tests];

    /* Extract and filter the tests based on the regex */
    regex_t reg;
    int error = regcomp(&reg, CONFIG_TESTPRINTER_REGEX, REG_EXTENDED | REG_NOSUB);
    if (error != 0) {
        printf("Error compiling regex \"%s\"\n", CONFIG_TESTPRINTER_REGEX);
        return;
    }

    int num_tests = 0;
    for (struct testcase *i = __start__test_case; i < __stop__test_case; i++) {
        if (regexec(&reg, i->name, 0, NULL, 0) == 0) {
             tests[num_tests] = i;
             num_tests++;
         }
     }
     
    regfree(&reg);

    /* Sort the tests to remove any non determinism in test ordering */
    qsort(tests, num_tests, sizeof(struct testcase*), test_comparator);

    /* Now that they are sorted we can easily ensure there are no duplicate tests.
     * this just ensures some sanity as if there are duplicates, they could have some
     * arbitrary ordering, which might result in difficulty reproducing test failures */
     for (int i = 1; i < num_tests; i++) {
         test_assert_fatal(strcmp(tests[i]->name, tests[i - 1]->name) != 0);
     }

     sel4test_start_suite(name);
     /* Run tests */
     for (int i = 0; i < num_tests; i++) {
         sel4test_start_test(tests[i]->name);
         int result = run_test(tests[i]);
         if (result != SUCCESS) {
             current_test_passed = 0;
         }
         sel4test_end_test();
     }

     /* Run the tests backwards... */
     for (int i = num_tests - 1; i >= 0; i--) {
         assert(tests[i]->name != NULL);
         /* junit doesn't like duplicate test names */
         int length = strlen(tests[i]->name);
         char name_copy[length + 2];
         strncpy(name_copy, tests[i]->name, length);
         name_copy[length] = '2';
         name_copy[length + 1] = '\0';
         sel4test_start_test(name_copy);
         int result = run_test(tests[i]);
         if (result != SUCCESS) {
              current_test_passed = 0;
         }
         sel4test_end_test();
     }
 
     sel4test_end_suite();

     /* Print closing banner. */
     printf("\n");
     int tests_run, passes, failures;
     sel4_test_get_suite_results(&tests_run, &passes, &failures);
     printf("%d/%d tests passed.\n", tests_run - failures, tests_run);
     if (failures > 0) {
         printf("*** FAILURES DETECTED ***\n");
     } else {
         printf("All is well in the universe.\n");
     }
     printf("\n\n");
}


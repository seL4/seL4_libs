/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef SEL4_TEST_H
#define SEL4_TEST_H

/* Include Kconfig variables. */
#include <autoconf.h>

#include <sel4/sel4.h>

#include <sel4test/prototype.h>
#include <sel4test/macros.h>

#include <stdio.h>
#include <string.h>

#define SUCCESS 0
#define FAILURE (-1)


/* Contains information about the test environment.
 * Define struct env in your application. */
struct env;
typedef struct env *env_t;

/* Prototype of a test function. Returns non-zero on failure. */
typedef int (*test_fn)(env_t, void *);

/* Represents a single testcase. */
typedef struct testcase {
    char *name;
    char *description;
    test_fn function;
    void *args;
} testcase_t;  

/* Declare a testcase. */
#define DEFINE_TEST(_name, _description, _function) \
    static __attribute__((used)) __attribute__((section("_test_case"))) struct testcase TEST_ ## _name = { \
    .name = #_name, \
    .description = _description, \
    .function = _function, \
}; \

static inline int
test_comparator(const void *a, const void *b)
{
    const struct testcase **ta = (const struct testcase**)a;
    const struct testcase **tb = (const struct testcase**)b;
    return strcmp((*ta)->name, (*tb)->name);
}

/* Fails a test case, stop running the rest of the test, but keep running other tests. */
static inline int _test_fail(char *condition, char *file, int line)
{
    _sel4test_failure(condition, file, line);
#ifdef CONFIG_TESTPRINTER_HALT_ON_TEST_FAILURE
    printf("Halting on first test failure...\n");
    sel4test_end_test();
    sel4test_end_suite();
#ifdef CONFIG_DEBUG_BUILD    
    seL4_DebugHalt();
#endif /* CONFIG_DEBUG_BUILD */
    while(1);
#endif /* CONFIG_TESTPRINTER_HALT_ON_TEST_FAILURE */
    return FAILURE;
}

/* Fails a test case, keep running the rest of the test, then keep running other tests. */
static inline void _test_error(char *condition, char *file, int line)
{

    _sel4test_report_error(condition, file, line);
#ifdef CONFIG_TESTPRINTER_HALT_ON_TEST_FAILURE
    printf("Halting on first test failure...\n");
    sel4test_end_test();
    sel4test_end_suite();
#ifdef CONFIG_DEBUG_BUILD    
    seL4_DebugHalt();
#endif /* CONFIG_DEBUG_BUILD */
    while(1);
#endif /* CONFIG_TESTPRINTER_HALT_ON_TEST_FAILURE */
 
}

/* Fails a test case, stop everything. */
static inline void _test_abort(char *condition, char *file, int line)
{
    _sel4test_failure(condition, file, line);
    printf("Halting on fatal assertion...\n");
    sel4test_end_test();
    sel4test_end_suite();
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugHalt();
#endif /* CONFIG_DEBUG_BUILD */
    while(1);
}

#define test_assert(e) if (!(e)) return _test_fail(#e, __FILE__, __LINE__)
#define test_check(e) if (!(e)) _test_error(#e, __FILE__, __LINE__)
#define test_assert_fatal(e) if (!(e)) _test_abort(#e, __FILE__, __LINE__)

env_t sel4test_get_env(void);

/*
 * Example basic run test
 */
static inline int
sel4test_basic_run_test(struct testcase *t) 
{
    return t->function(sel4test_get_env(), t->args);
}

/* 
 * Run every test defined with the DEFINE_TEST macro.
 *
 * Use CONFIG_TESTPRINTER_REGEX to filter tests.
 *
 * @param name the name of the test suite
 * @param run_test function that runs the tests. 
 *
 */
void sel4test_run_tests(char *name, int (*run_test)(struct testcase *t));

/* 
 * Get a testcase.
 *
 * @param name the name of the test to retrieve.
 * @return the test corresponding to name, NULL if test not found.
 */
testcase_t* sel4test_get_test(char *name);
#endif /* SEL4_TEST_H */

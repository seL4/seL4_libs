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

#ifndef SEL4_TEST_H
#define SEL4_TEST_H

/* Include Kconfig variables. */
#include <autoconf.h>

#include <sel4/sel4.h>

#include <sel4test/prototype.h>
#include <sel4test/macros.h>
#include <utils/attribute.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SUCCESS true
#define FAILURE false

/* Contains information about the test environment.
 * Define struct env in your application. */
struct env;
typedef struct env *env_t;

/* Prototype of a test function. Returns false on failure. */
typedef int (*test_fn)(env_t);

/* Test type definitions. */
typedef enum test_type_name {BOOTSTRAP = 0, BASIC} test_type_name_t;
typedef struct testcase testcase_t; // Forward type declaration.
typedef struct test_type {
    /* Represents a single test type. See comment for `struct testcase` for info about ALIGN(32). */
    const char *name;
    test_type_name_t id;
    // Function called before and after all the tests for this test type have been run.
    void (*set_up_test_type)(env_t e);
    void (*tear_down_test_type)(env_t e);
    // Function called before and after each test for this test type.
    void (*set_up)(env_t e);
    void (*tear_down)(env_t e);
    // Run the test.
    int (*run_test)(struct testcase* test, env_t e);
} ALIGN(32) test_type_t;

/* Declare a test type.
 * For now, we put the test types in a separate elf section. */
#define DEFINE_TEST_TYPE(_name, _id, _set_up_test_type, _tear_down_test_type, _set_up, _tear_down, _run_test) \
    __attribute__((used)) __attribute__((section("_test_type"))) struct test_type TEST_TYPE_ ##_name = { \
    .name = #_name, \
    .id = _id, \
    .set_up_test_type = _set_up_test_type, \
    .tear_down_test_type = _tear_down_test_type, \
    .set_up = _set_up, \
    .tear_down = _tear_down, \
    .run_test = _run_test, \
};

/* Represents a single testcase.
 * Because this struct is used to declare variables that get
 * placed into custom sections, that we later treat as an array,
 * we need to provide some additional alignment. The reason for
 * this is because these are independent static variables gcc
 * is permitted to place arbitrary padding between them. This
 * is annoying as we wish to treat all the items in the section
 * as an array. To work around this we use the fact that GCC
 * seems to be wanting to align to 32byte boundaries and set
 * the struct alignment to 32. Therefore the actual size of
 * the objects in the section is the same as the size reported
 * by sizeof(struct testcase), allowing as to treat the items
 * in the section as an array */
typedef struct testcase {
    const char *name;
    const char *description;
    test_fn function;
    test_type_name_t test_type;
} ALIGN(32) testcase_t;

/* Declare a testcase. */
#define DEFINE_TEST_WITH_TYPE(_name, _description, _function, _test_type) \
    __attribute__((used)) __attribute__((section("_test_case"))) struct testcase TEST_ ## _name = { \
    .name = #_name, \
    .description = _description, \
    .function = _function, \
    .test_type = _test_type, \
};

#define DEFINE_TEST(_name, _description, _function) DEFINE_TEST_WITH_TYPE(_name, _description, _function, BASIC)

#define DEFINE_TEST_BOOTSTRAP(_name, _description, _function) DEFINE_TEST_WITH_TYPE(_name, _description, _function, BOOTSTRAP)
/**/

static inline int
test_type_comparator(const void *a, const void *b)
{
    const struct test_type **ta = (const struct test_type**) a;
    const struct test_type **tb = (const struct test_type**) b;
    if ((*ta)->id > (*tb)->id) {
        return 1;
    } else if ((*ta)->id < (*tb)->id) {
        return -1;
    }

    return 0;
}

static inline int
test_comparator(const void *a, const void *b)
{
    const struct testcase **ta = (const struct testcase**)a;
    const struct testcase **tb = (const struct testcase**)b;
    return strcmp((*ta)->name, (*tb)->name);
}

/* Fails a test case, stop running the rest of the test, but keep running other tests. */
static inline int _test_fail(const char *condition, const char *file, int line)
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
static inline void _test_error(const char *condition, const char *file, int line)
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
static inline void _test_abort(const char *condition, const char *file, int line)
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

#define __TEST_BUFFER_SIZE 200
#define test_op_type(a, b, op, t, name_a, name_b, cast) \
    do {\
        if (!(a op b)) { \
            int len = snprintf(NULL, 0, "Check %s(" t ") %s %s(" t ") failed.",\
                               #name_a, (cast) a, #op, #name_b, (cast) b) + 1; \
            char buffer[len]; \
            snprintf(buffer, len, "Check %s(" t ") %s %s(" t ") failed.",\
                     #name_a, (cast) a, #op, #name_b, (cast) b); \
            _test_error(buffer, __FILE__, __LINE__); \
        }\
    } while (0)

#define test_op(a, b, op) \
    do { \
         typeof (a) _a = (a); \
         typeof (b) _b = (b); \
         if (sizeof(_a) != sizeof(_b)) { \
             int len = snprintf(NULL, 0, "%s (size %zu) != %s (size %zu), use of test_eq incorrect", #a,\
                     sizeof(_a), #b, sizeof(_b)) + 1;\
             char buffer[len];\
             snprintf(buffer, len, "%s (size %zu) != %s (size %zu), use of test_eq incorrect", #a, sizeof(_a),\
                     #b, sizeof(_b));\
             _test_error(buffer, __FILE__, __LINE__);\
         } else if (TYPES_COMPATIBLE(typeof(_a), int)) {\
             test_op_type(_a, _b, op, "%d", a, b, int); \
         } else if (TYPES_COMPATIBLE(typeof(_a), long)) {\
             test_op_type(_a, _b, op, "%ld", a, b, long); \
         } else if (TYPES_COMPATIBLE(typeof(_a), long long)) {\
             test_op_type(_a, _b, op, "%lld", a, b, long long); \
         } else if (TYPES_COMPATIBLE(typeof(_a), unsigned int)) {\
             test_op_type(_a, _b, op, "%u", a, b, unsigned int); \
         } else if (TYPES_COMPATIBLE(typeof(_a), unsigned long)) {\
             test_op_type(_a, _b, op, "%lu", a, b, unsigned long); \
         } else if (TYPES_COMPATIBLE(typeof(_a), unsigned long long)) {\
             test_op_type(_a, _b, op, "%llu", a, b, unsigned long long); \
         } else if (TYPES_COMPATIBLE(typeof(_a), char)) {\
             test_op_type(_a, _b, op, "%c", a, b, char); \
         } else if (TYPES_COMPATIBLE(typeof(_a), uintptr_t)) {\
             test_op_type(_a, _b, op, "0x%" PRIxPTR, a, b, uintptr_t);\
         } else { \
             _test_error("Cannot use test_op on this type", __FILE__, __LINE__);\
         }\
    } while (0)

/* Pretty printed test_check wrapper macros for basic comparisons on base types,
 * which output the values and variable names to aid debugging */
#define test_eq(a, b)  test_op(a, b, ==)
#define test_neq(a, b) test_op(a, b, !=)
#define test_ge(a, b)  test_op(a, b, >)
#define test_geq(a, b) test_op(a, b, >=)
#define test_le(a, b)  test_op(a, b, <)
#define test_leq(a, b) test_op(a, b, <=)

#define __TEST_MAX_STRING 50
#define test_strop(a, b, op) \
     do {\
          if (strnlen(a, __TEST_MAX_STRING) == __TEST_MAX_STRING) {\
              _test_error("String " #a " too long for test_str* macros", __FILE__, __LINE__);\
          } else if (strnlen(b, __TEST_MAX_STRING) == __TEST_MAX_STRING) {\
              _test_error("String " #b " too long for test_str* macros", __FILE__, __LINE__);\
          } else if (!(strncmp(a, b, __TEST_MAX_STRING)) op 0) {\
              char buffer[__TEST_BUFFER_SIZE + 2 * __TEST_MAX_STRING];\
              snprintf(buffer, sizeof(buffer),\
                       "Check %s(%s) %s %s(%s) failed.", #a, a, #op, #b, b);\
              _test_error(buffer, __FILE__, __LINE__); \
          }\
    } while (0)

/* Pretty printed test_check wrapper macros for basic comparisons on c strings,
 * which output the values and variable names to aid debugging */
#define test_streq(a, b)  test_strop(a, b, ==)
#define test_strneq(a, b) test_strop(a, b, !=)
#define test_strge(a, b)  test_strop(a, b, >)
#define test_strgeq(a, b) test_strop(a, b, >=)
#define test_strle(a, b)  test_strop(a, b, <)
#define test_strleq(a, b) test_strop(a, b, <=)

env_t sel4test_get_env(void);

/*
 * Example basic run test
 */
static inline int
sel4test_basic_run_test(struct testcase *t)
{
    return t->function(sel4test_get_env());
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
void sel4test_run_tests(const char *name, int (*run_test)(struct testcase *t));

/*
 * Get a testcase.
 *
 * @param name the name of the test to retrieve.
 * @return the test corresponding to name, NULL if test not found.
 */
testcase_t* sel4test_get_test(const char *name);

bool sel4test_get_result(void);
#endif /* SEL4_TEST_H */

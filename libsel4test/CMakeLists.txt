#
# Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.7.2)

project(libsel4test C)

set(configure_string "")

config_string(
    LibSel4TestPrinterRegex
    TESTPRINTER_REGEX
    "A POSIX regex pattern that is used to filter the tests to run"
    DEFAULT
    ".*"
)

config_option(
    LibSel4TestPrinterHaltOnTestFailure
    TESTPRINTER_HALT_ON_TEST_FAILURE
    "Halt on the first test failure \
    Default behaviour is to try and run the test suite, regardless of any failures. \
    Turning this on will stop the test suite on the first error"
    DEFAULT
    OFF
)
config_option(LibSel4TestPrintXML PRINT_XML "Print XML output. This will \
    buffer the test output and print in <stdout> blocks so that bamboo \
    can parse the test logs properly. Turn this off for debugging your test suite" DEFAULT OFF)
mark_as_advanced(LibSel4TestPrinterRegex LibSel4TestPrinterHaltOnTestFailure LibSel4TestPrintXML)
add_config_library(sel4test "${configure_string}")

file(GLOB deps src/*.c)

list(SORT deps)

add_library(sel4test STATIC EXCLUDE_FROM_ALL ${deps})
target_include_directories(sel4test PUBLIC include)
target_link_libraries(
    sel4test
    muslc
    sel4
    utils
    sel4vka
    sel4vspace
    sel4platsupport
    sel4rpc
    sel4sync
    sel4simple
    sel4utils
    sel4_autoconf
    sel4test_Config
)

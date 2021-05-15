#
# Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.7.2)

project(libsel4serialserver C)

set(configure_string "")

config_option(
    LibSel4SerialServerColoredOutput
    SERIAL_SERVER_COLOURED_OUTPUT
    "Annotate output with different ansi colours for each client"
    DEFAULT
    ON
)
mark_as_advanced(LibSel4SerialServerColoredOutput)
add_config_library(sel4serialserver "${configure_string}")

set(deps src/clientapi.c src/parentapi.c src/server.c)

add_library(sel4serialserver STATIC EXCLUDE_FROM_ALL ${deps})
target_include_directories(sel4serialserver PUBLIC include)
target_link_libraries(
    sel4serialserver
    PUBLIC
        muslc
        sel4
        sel4vspace
        sel4simple
        sel4platsupport
        utils
        sel4utils
        sel4vka
    PRIVATE sel4_autoconf sel4serialserver_Config
)

add_library(sel4serialserver_tests STATIC EXCLUDE_FROM_ALL src/test.c)
target_link_libraries(sel4serialserver_tests sel4serialserver sel4test)

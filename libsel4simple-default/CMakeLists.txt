#
# Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.7.2)

project(libsel4simple-default C)

file(GLOB deps src/*.c src/arch/${KernelArch}/*.c)

list(SORT deps)

add_library(sel4simple-default STATIC EXCLUDE_FROM_ALL ${deps})
target_include_directories(sel4simple-default PUBLIC include)
target_link_libraries(
    sel4simple-default
    PUBLIC
        muslc
        sel4
        sel4simple
        sel4debug
        sel4vspace
    PRIVATE sel4_autoconf
)

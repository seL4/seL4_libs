#
# Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.7.2)

project(libsel4ync C)

file(GLOB deps src/*.c)

list(SORT deps)

add_library(sel4sync STATIC EXCLUDE_FROM_ALL ${deps})

target_include_directories(sel4sync PUBLIC include)

target_link_libraries(
    sel4sync
    PUBLIC
        muslc
        sel4
        sel4vka
        platsupport
        utils
        sel4_autoconf
)

if(KernelDebugBuild)
    target_link_libraries(sel4sync PUBLIC sel4debug)
endif()

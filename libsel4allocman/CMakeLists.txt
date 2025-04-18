#
# Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.16.0)

project(libsel4allocman C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")

file(
    GLOB
        deps
        src/*.c
        src/cspace/*.c
        src/utspace/*.c
        src/mspace/*.c
)

list(SORT deps)

add_library(sel4allocman STATIC EXCLUDE_FROM_ALL ${deps})
target_include_directories(sel4allocman PUBLIC include "sel4_arch/${KernelSel4Arch}")
target_include_directories(sel4allocman PUBLIC include "arch/${KernelArch}")
target_link_libraries(
    sel4allocman
    PUBLIC
        muslc
        sel4
        sel4vka
        sel4utils
        sel4vspace
        sel4_autoconf
)

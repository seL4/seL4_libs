#
# Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.7.2)

project(libsel4vspace C)

file(
    GLOB
        deps
        src/*.c
        src/arch/${KernelArch}/*.c
        src/sel4_arch/${KernelSel4Arch}/*.c
)

list(SORT deps)

add_library(sel4vspace STATIC EXCLUDE_FROM_ALL ${deps})
target_include_directories(sel4vspace PUBLIC include "arch_include/${KernelArch}")
target_link_libraries(sel4vspace muslc sel4 sel4vka sel4_autoconf sel4utils_Config)

#
# Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.16.0)

project(libsel4utils C)

set(configure_string "")

config_string(
    LibSel4UtilsStackSize
    SEL4UTILS_STACK_SIZE
    "Size of stacks in bytes to allocate if using vspace interface in this library"
    DEFAULT
    65536
    UNQUOTE
)
config_string(
    LibSel4UtilsCSpaceSizeBits
    SEL4UTILS_CSPACE_SIZE_BITS
    "Size of default cspace to spawn processes with"
    DEFAULT
    12
    UNQUOTE
)
config_option(LibSel4UtilsProfile SEL4UTILS_PROFILE "Profiling tools \
    Enables the functionality of a set of profiling tools. When disabled these profiling tools \
    will compile down to nothing." DEFAULT OFF)
mark_as_advanced(LibSel4UtilsStackSize LibSel4UtilsCSpaceSizeBits LibSel4UtilsProfile)
add_config_library(sel4utils "${configure_string}")

file(
    GLOB
        deps
        src/*.c
        src/vspace/*.c
        src/irq_server/*.c
        src/serial_server/*.c
        src/time_server/*.c
        src/sel4_arch/${KernelSel4Arch}/*.c
        src/arch/${KernelArch}/*.c
)

list(SORT deps)

add_library(sel4utils STATIC EXCLUDE_FROM_ALL ${deps})
target_include_directories(
    sel4utils
    PUBLIC include "sel4_arch_include/${KernelSel4Arch}" "arch_include/${KernelArch}"
)
target_link_libraries(
    sel4utils
    muslc
    sel4
    sel4runtime
    sel4vspace
    sel4simple
    sel4platsupport
    elf
    cpio
    sel4utils_Config
    sel4_autoconf
)

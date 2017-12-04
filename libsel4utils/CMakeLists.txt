#
# Copyright 2017, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
#

cmake_minimum_required(VERSION 3.7.2)

project(libsel4utils C)

set(configure_string "")

config_string(LibSel4UtilsStackSize SEL4UTILS_STACK_SIZE
    "Size of stacks in bytes to allocate if using vspace interface in this library"
    DEFAULT 65536
    UNQUOTE
)
config_string(LibSel4UtilsCSpaceSizeBits SEL4UTILS_CSPACE_SIZE_BITS
    "Size of default cspace to spawn processes with"
    DEFAULT 12
    UNQUOTE
)
config_option(LibSel4UtilsProfile SEL4UTILS_PROFILE "Profiling tools \
    Enables the functionality of a set of profiling tools. When disabled these profiling tools \
    will compile down to nothing."
    DEFAULT OFF
)

add_config_library(sel4utils "${configure_string}")

file(GLOB deps src/*.c src/vspace/*.c src/irq_server/*.c src/serial_server/*.c src/time_server/*.c src/sel4_arch/${KernelSel4Arch}/*.c)

list(SORT deps)

add_library(sel4utils STATIC EXCLUDE_FROM_ALL ${deps})
target_include_directories(sel4utils PUBLIC include "sel4_arch_include/${KernelSel4Arch}" "arch_include/${KernelArch}")
target_link_libraries(sel4utils Configuration muslc sel4 sel4vspace sel4simple sel4platsupport elf cpio)

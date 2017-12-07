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

project(libsel4bench C)

file(GLOB deps
    src/*.c
    src/arch/${KernelArch}/*.c
    src/arch/${KernelArch}/armv/${KernelArmArmV}/*.c
    src/arch/${KernelArch}/cpu/${KernelArmCPU}/*.c
)

list(SORT deps)

add_library(sel4bench EXCLUDE_FROM_ALL ${deps})
target_include_directories(sel4bench PUBLIC
    include
    "arch_include/${KernelArch}/armv/${KernelArmArmV}"
    "arch_include/${KernelArch}/cpu/${KernelArmCPU}"
    "sel4_arch_include/${KernelSel4Arch}/"
    "arch_include/${KernelArch}"
    src
    )
if (KernelArmArmV STREQUAL "armv7ve")
    target_include_directories(sel4bench PUBLIC "arch_include/${KernelArch}/armv/armv7-a")
endif()
target_link_libraries(sel4bench Configuration muslc sel4 utils sel4benchsupport)

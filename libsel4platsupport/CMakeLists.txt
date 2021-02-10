#
# Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.7.2)

project(libsel4platsupport C ASM)

set(configure_string "")

config_option(
    LibSel4PlatSupportUseDebugPutChar
    LIB_SEL4_PLAT_SUPPORT_USE_SEL4_DEBUG_PUTCHAR
    "Redirect putchar() to seL4_DebugPutchar()"
    DEFAULT
    ON
    DEPENDS
    "KernelPrinting"
    DEFAULT_DISABLED
    OFF
)

config_option(
    LibSel4PlatSupportStart
    LIB_SEL4_PLAT_SUPPORT_START
    "Supply _start entry point. Emit _start with a default definition that sets up a \
    stack and does what's required to enter main()."
    DEFAULT
    OFF
)

config_option(
    LibSel4SupportSel4Start
    LIB_SEL4_PLAT_SUPPORT_SEL4_START
    "Supply _sel4_start entry point. Emit a _sel4_start that sets up a stack and does \
    what's required to enter a regular _start routine"
    DEFAULT
    ON
)
mark_as_advanced(LibSel4PlatSupportUseDebugPutChar LibSel4PlatSupportStart LibSel4SupportSel4Start)
add_config_library(sel4platsupport "${configure_string}")

if(KernelArchRiscV)
    set(kernel_arch arm)
else()
    set(kernel_arch ${KernelArch})
endif()

file(
    GLOB
        deps
        src/*.c
        src/cspace/*.c
        src/arch/${kernel_arch}/*.c
        src/plat/${KernelPlatform}/*.c
        src/mach/${KernelArmMach}/*.c
        src/sel4_arch/${KernelSel4Arch}/*.S
        src/arch/${kernel_arch}/*.S
)

list(SORT deps)

add_library(sel4platsupport STATIC EXCLUDE_FROM_ALL ${deps})
target_include_directories(
    sel4platsupport
    PUBLIC include "arch_include/${kernel_arch}" plat_include/${KernelPlatform}
)
if(NOT "${KernelArmMach}" STREQUAL "")
    target_include_directories(sel4platsupport PUBLIC include "mach_include/${KernelArmMach}")
endif()
target_link_libraries(
    sel4platsupport
    PUBLIC
        muslc
        sel4
        sel4runtime
        sel4simple
        utils
        sel4vspace
        platsupport
        sel4simple-default
    PRIVATE sel4platsupport_Config sel4muslcsys_Config sel4_autoconf
)

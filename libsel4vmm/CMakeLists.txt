#
# Copyright 2018, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
#

cmake_minimum_required(VERSION 3.8.2)

project(libsel4vmm C)

set(configure_string "")

config_option(LibSel4VMMVESAFrameBuffer VMM_VESA_FRAMEBUFFER "Expose VESA Frame Buffer
    Support for mapping the VESA frame buffer frames into the guest OS.
    This option is useful for working around non-working video drivers otherwise,
    due to missing features or such." DEFAULT ON)

config_option(LibSel4VMMDebug LIB_VMM_DEBUG "Enable VMM debug output" DEFAULT ON)

config_string(LibSel4VMMDebugLevel LIB_VMM_DEBUG_LEVEL "VMM Debug Output
    Debug verbosity level.
    5 levels for debug messages:
       0: Always printout
       1: Main entry point in a module
       2: 2nd level entry point in a module
       3: Main entry point of a function
       4: Details inside a function" DEFAULT 3 UNQUOTE)

config_option(
    LibSel4VMMVMXTimerDebug
    LIB_VMM_VMX_TIMER_DEBUG
    "Use VMX Pre-Emption timer for debugging
    Will cause a regular vmexit to happen based on VMX pre-emption
    timer. At each exit the guest state will be printed out. This
    can be used to aid debugging when running a guest causes nothing
    to appear to happen"
    DEFAULT
    OFF
)

config_string(
    LibSel4VMMVMXTimerTimeout
    LIB_VMM_VMX_TIMER_TIMEOUT
    "Cycles between VMX timer pre-emptions
    Cycles between timer exits. Time spent loading the VMCS context
    count, so setting this too low may result in the guest making
    no progress"
    DEFAULT
    4000
    DEPENDS
    "LibSel4VMMVMXTimerDebug"
)

config_option(LibSel4VMMIgnoreEPTViolation VMM_IGNORE_EPT_VIOLATION "Ignore EPT Violations
    If set then EPT faults will be ignored and the guest will be resumed" DEFAULT OFF)
mark_as_advanced(
    LibSel4VMMVESAFrameBuffer
    LibSel4VMMDebug
    LibSel4VMMDebugLevel
    LibSel4VMMVMXTimerDebug
    LibSel4VMMVMXTimerTimeout
    LibSel4VMMIgnoreEPTViolation
)
add_config_library(sel4vmm "${configure_string}")

add_compile_options(-std=gnu99)

file(
    GLOB
        sources
        src/vmm/*.c
        src/platform/*.c
        src/driver/*.c
        src/manager/*.c
        src/processor/*.c
)

add_library(sel4vmm STATIC EXCLUDE_FROM_ALL ${sources})

target_include_directories(sel4vmm PUBLIC include)
target_link_libraries(
    sel4vmm
    PUBLIC
        muslc
        sel4
        sel4utils
        pci
        sel4allocman
        ethdrivers
        platsupport
        sel4vmm_Config
        sel4_autoconf
)

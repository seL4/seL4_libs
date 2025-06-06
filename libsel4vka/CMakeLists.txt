#
# Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.16.0)

project(libsel4vka C)

set(configure_string "")

config_option(LibVKAAllowMemoryLeaks LIB_VKA_ALLOW_MEMORY_LEAKS "Leak memory on free \
    Changes the default free functions to silently leak memory \
    instead of assert failing. \
    This is useful in scenarios where code is correctly freeing \
    resources, but you want to use an allocator that does not \
    implement free." DEFAULT OFF)

config_string(
    LibVKADebugLiveSlotsSZ
    LIB_SEL4_VKA_DEBUG_LIVE_SLOTS_SZ
    "(debug allocator) live slot buffer size \
    VKA contains an allocator, debugvka, that wraps a target VKA allocator \
    for the purposes of debugging. It tracks slot and object allocations \
    and catches both caller problems like double free, and allocator \
    problems like handing out the same resource twice. This configuration \
    option sets the maximum number of CSlots that can be tracked at any one \
    time."
    DEFAULT
    0
    UNQUOTE
)

config_string(
    LibVKADebugLiveObjsSZ
    LIB_SEL4_VKA_DEBUG_LIVE_OBJS_SZ
    "(debug allocator) live object buffer size \
    As for the above option, but sets the maximum number of objects that \
    can be tracked at any one time."
    DEFAULT
    0
    UNQUOTE
)
mark_as_advanced(LibVKAAllowMemoryLeaks LibVKADebugLiveSlotsSZ LibVKADebugLiveObjsSZ)
add_config_library(sel4vka "${configure_string}")

file(GLOB deps src/*.c)

list(SORT deps)

add_library(sel4vka STATIC EXCLUDE_FROM_ALL ${deps})
target_include_directories(
    sel4vka
    PUBLIC include "sel4_arch_include/${KernelSel4Arch}" "arch_include/${KernelArch}"
)
target_link_libraries(
    sel4vka
    PUBLIC
        muslc
        sel4
        utils
        sel4vka_Config
        sel4_autoconf
)
if(KernelDebugBuild)
    target_link_libraries(sel4vka PUBLIC sel4debug)
endif()

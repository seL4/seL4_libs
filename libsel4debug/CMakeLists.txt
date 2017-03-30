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

project(libsel4debug C ASM)

set(configure_string "")

config_string(LibSel4DebugAllocBufferEntries LIBSEL4DEBUG_ALLOC_BUFFER_ENTRIES
    "Pointer tracking buffer entries \
    Size of buffer to use for tracking memory allocations within \
    instrumentation. This setting has no effect if you are not using the \
    allocation instrumentation. Setting this value to 0 disables pointer \
    tracking."
    DEFAULT 0
    UNQUOTE
)

config_choice(LibSel4DebugFunctionInstrumentation LIB_SEL4_DEBUG_FUNCTION_INSTRUMENTAITON
    "Function instrumentation to apply to function calls. This setting will only have \
    an effect when your CFLAGS include -finstrument-functions, in which \
    case __cyg_profile_func_enter will be called on every function entry \
    and __cyg_profile_func_exit will be called on every function exit. The \
    value of this setting determines the implementation of \
    __cyg_profile_func_enter and __cyg_profile_func_exit supplied by this \
    library. Regardless of the option you select, you can always override \
    the implementation by providing your own in your application. \
    none -> \
        No implementation of __cyg_profile_func_enter and \
        __cyg_profile_func_exit are provided. \
    printf -> \
        Print entry and exit of every function call. This option is useful for \
        a very coarse and noisy description of what your code is doing. \
    backtrace -> \
        Track function calls for the purposes of a backtrace. You will need to \
        enable this option if you want to retrieve programmatic backtraces."
    "none;LibSel4DebugFunctionInstrumentationNone;LIBSEL4DEBUG_FUNCTION_INSTRUMENTATION_NONE"
    "printf;LibSel4DebugFunctionInstrumentationPrintf;LIBSEL4DEBUG_FUNCTION_INSTRUMENTATION_TRACE"
    "backtrace;LibSel4DebugFunctionInstrumentationBackgrace;LIBSEL4DEBUG_FUNCTION_INSTRUMENTATION_BACKTRACE"
)

add_config_library(sel4debug "${configure_string}")

file(GLOB deps src/*.c src/sel4_arch/${KernelSel4Arch}/*.c src/plat/${KernelPlatform}/*.c src/sel4_arch/${KernelSel4Arch}/*.S)

list(SORT deps)

add_library(sel4debug STATIC EXCLUDE_FROM_ALL ${deps})
target_include_directories(sel4debug PUBLIC include "arch_include/${KernelArch}" sel4_arch_include/${KernelSel4Arch})
target_link_libraries(sel4debug Configuration muslc sel4 utils)

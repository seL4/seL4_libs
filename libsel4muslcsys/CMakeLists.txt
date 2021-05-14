#
# Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.7.2)

project(libsel4muslcsys C)

set(configure_string "")

config_string(LibSel4MuslcSysMorecoreBytes LIB_SEL4_MUSLC_SYS_MORECORE_BYTES "Malloc limit \
    This value sets the number of bytes reserved for the region malloc \
    allocates from. Note that you won't actually be able to allocate this \
    entire amount because there is some bookkeeping overhead. This area is \
    allocated statically." DEFAULT 1048576 UNQUOTE)

config_option(
    LibSel4MuslcSysDebugHalt
    LIB_SEL4_MUSLC_SYS_DEBUG_HALT
    "Perform seL4_DebugHalt on _exit and _abort \
    Controls whether or not a DebugHalt should be called to implement Exit \
    style syscalls. This is useful if you have a single app and thread, but \
    should be turned off if using multiple threads that may wish to exit \
    independently of the whole system"
    DEFAULT
    ON
)

config_option(
    LibSel4MuslcSysCPIOFS
    LIB_SEL4_MUSLC_SYS_CPIO_FS
    "Implementation of a simple file system usi g CPIO archives \
    If this is enabled open and read syscalls will attempt to use the cpio archive \
    _cpio_archive. This implements a basic read only POSIX interface to that file system"
    DEFAULT
    OFF
)

config_option(
    LibSel4MuslcSysArchPutcharWeak
    LIB_SEL4_MUSLC_SYS_ARCH_PUTCHAR_WEAK
    "Make __arch_putchar a weak symbol \
    This allows you to override __arch_putchar in another library or your \
    application."
    DEFAULT
    OFF
)

config_string(
    LibSel4MuslcSysConstructorPriority
    LIB_SEL4_MUSLC_SYS_CONSTRUCTOR_PRIORITY
    "Set the priority of the muslc initialisation constructors such that
	they occur after external initialisation of the system call
	dependencies."
    DEFAULT
    "MUSLCSYS_WITH_VSYSCALL_PRIORITY + 10"
    UNQUOTE
)
mark_as_advanced(
    LibSel4MuslcSysMorecoreBytes
    LibSel4MuslcSysDebugHalt
    LibSel4MuslcSysCPIOFS
    LibSel4MuslcSysArchPutcharWeak
)
add_config_library(sel4muslcsys "${configure_string}")

file(GLOB deps src/*.c)

list(SORT deps)

# TODO: This use to be calculated by the following line. Need to use a generator expression and generate
# this into a header file at build time
# MUSLC_HIGHEST_SYSCALL := $(shell cat $(STAGE_DIR)/include/bits/syscall.h | sed 's/^.*[^0-9]\([0-9]*\)$$/\1/' | sort -nr | head -1)
set(HighestSyscall 400)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_XOPEN_SOURCE=700 -DMUSLC_HIGHEST_SYSCALL=${HighestSyscall}")

add_library(sel4muslcsys STATIC EXCLUDE_FROM_ALL ${deps})
# Force the muslcsys_init_muslc constructor to be included in dependants
target_link_options(sel4muslcsys BEFORE INTERFACE "-Wl,-umuslcsys_init_muslc")
target_include_directories(sel4muslcsys PUBLIC include)
target_link_libraries(
    sel4muslcsys
    PUBLIC
        # These quotes are needed to prevent cmake splitting these arguments
        "-Wl,-u -Wl,__vsyscall_ptr"
        muslc
        sel4
        cpio
        utils
        sel4utils
        sel4muslcsys_Config
    PRIVATE sel4_autoconf
)

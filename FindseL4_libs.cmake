#
# Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

set(SEL4_LIBS_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE STRING "")
mark_as_advanced(SEL4_LIBS_DIR)

macro(sel4_libs_import_libraries)
    add_subdirectory(${SEL4_LIBS_DIR} seL4_libs)
endmacro()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(seL4_libs DEFAULT_MSG SEL4_LIBS_DIR)

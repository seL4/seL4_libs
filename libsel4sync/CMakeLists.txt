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

project(libsel4ync C)

file(GLOB deps src/*.c)

list(SORT deps)

add_library(sel4sync STATIC EXCLUDE_FROM_ALL ${deps})
target_include_directories(sel4sync PUBLIC include)
target_link_libraries(sel4sync Configuration muslc sel4 sel4vka platsupport utils)

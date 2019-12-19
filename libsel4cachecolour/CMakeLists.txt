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

project(libsel4cachecolour C ASM)

set(configure_string "")

config_string(
	LibSel4CacheColourNumCacheColours
	NUM_CACHE_COLOURS
	"Number of cache colours"
    DEFAULT
    32
    UNQUOTE
)

config_string(
	LibSel4CacheColourAllocSizeBits
	COLOUR_ALLOC_SIZEBITS
	"The largest size of untyped the cache colouring allocator \
	will manage"
	DEFAULT
	17
	UNQUOTE
)

config_string(
	LibSel4CacheColourMSpaceReserves
	COLOUR_MSPACE_RESERVES
	"Number of chunks reserved for the untyped notes in mspace"
	DEFAULT
	10
	UNQUOTE
)

mark_as_advanced(
	CLEAR
	LibSel4CacheColourNumCacheColours
	LibSel4CacheColourAllocSizeBits
	LibSel4CacheColourMSpaceReserves
)
add_config_library(sel4cachecolour "${configure_string}")

add_library(
	sel4cachecolour
	STATIC EXCLUDE_FROM_ALL "src/color_allocator.c" "src/vka.c"
)
target_include_directories(
    sel4cachecolour
    PUBLIC include
)
target_link_libraries(
	sel4cachecolour
	PUBLIC
	sel4
	muslc
	utils
	sel4vka
	sel4allocman
	sel4cachecolour_Config
	PRIVATE sel4_autoconf
)

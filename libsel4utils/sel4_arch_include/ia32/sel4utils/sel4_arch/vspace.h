/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <autoconf.h>
#include <sel4utils/gen_config.h>

#ifdef CONFIG_PAE_PAGING
#define VSPACE_MAP_PAGING_OBJECTS 3
#else
#define VSPACE_MAP_PAGING_OBJECTS 2
#endif

#define VSPACE_LEVEL_BITS 10
#define VSPACE_NUM_LEVELS 2


/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <autoconf.h>


#ifdef CONFIG_ARM_HYPERVISOR_SUPPORT

#if defined(CONFIG_ARM_PA_SIZE_BITS_44)

#define VSPACE_NUM_LEVELS       4

#elif defined(CONFIG_ARM_PA_SIZE_BITS_40)

#define VSPACE_NUM_LEVELS 3

#else
#error Unspecified PA size bits
#endif

#else /* CONFIG_ARM_HYPERVISOR_SUPPORT */

#define VSPACE_NUM_LEVELS 4

#endif /* end of !CONFIG_ARM_HYPERVISOR_SUPPORT */

#define VSPACE_MAP_PAGING_OBJECTS   5
#define VSPACE_LEVEL_BITS 9

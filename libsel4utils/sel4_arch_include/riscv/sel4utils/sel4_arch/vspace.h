/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef SEL4UTILS_SEL4_ARCH_VSPACE_H
#define SEL4UTILS_SEL4_ARCH_VSPACE_H

#include <autoconf.h>

#if CONFIG_PT_LEVELS == 2
#define KERNEL_RESERVED_START 0x7ffff000
#define VSPACE_LEVEL_BITS 10
#elif CONFIG_PT_LEVELS == 3
#define KERNEL_RESERVED_START 0x0000002ffffff000
#define VSPACE_LEVEL_BITS 9
#elif CONFIG_PT_LEVELS == 4
#define KERNEL_RESERVED_START 0x00007ffffffff000
#define VSPACE_LEVEL_BITS 9
#else
#error "Unsupported PT level"
#endif

#define VSPACE_MAP_PAGING_OBJECTS 5

#define VSPACE_NUM_LEVELS CONFIG_PT_LEVELS

#endif /* SEL4UTILS_SEL4_ARCH_VSPACE_H */

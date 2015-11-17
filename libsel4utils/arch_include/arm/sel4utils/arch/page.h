/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef _SEL4UTILS_ARCH_PAGE_H
#define _SEL4UTILS_ARCH_PAGE_H

#include <stdint.h>
#include <utils/attribute.h>
#include <sel4/arch/types.h>

/* ordered list of page sizes for this architecture */
static const UNUSED size_t utils_page_sizes[] = {
    seL4_PageBits,
    16,
#ifdef ARM_HYP
    21,
    25,
#else
    20,
    24
#endif /* ARM_HYP */
};

#endif /* _SEL4UTILS_ARCH_PAGE_H */

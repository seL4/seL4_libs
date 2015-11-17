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
#include <util/attributes.h>
#include <sel4/arch/types.h>

/* ordered list of page sizes for this architecture */
static const UNUSED size_t utils_page_sizes[] = {
    seL4_PageBits,
    seL4_LargePageBits,
};

#endif /* _SEL4UTILS_ARCH_PAGE_H */

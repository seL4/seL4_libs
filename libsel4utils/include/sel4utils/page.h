/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _SEL4UTILS_PAGE_H_
#define _SEL4UTILS_PAGE_H_

#include <vspace/page.h>
#include <utils/arith.h>

#define UTILS_NUM_PAGE_SIZES SEL4_NUM_PAGE_SIZES

static inline DEPRECATED("Use sel4_valid_size_bits") bool
utils_valid_size_bits(size_t size_bits)
{
    return sel4_valid_size_bits(size_bits);
}

#endif /* _SEL4UTILS_PAGE_H_ */

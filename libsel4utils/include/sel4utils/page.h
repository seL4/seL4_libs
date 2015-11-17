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

#include <sel4utils/arch/page.h>
#include <utils/arith.h>

#define UTILS_NUM_PAGE_SIZES ((int) ARRAY_SIZE(utils_page_sizes))

static inline bool
utils_valid_size_bits(size_t size_bits)
{
    for (int i = 0; i < UTILS_NUM_PAGE_SIZES && size_bits >= utils_page_sizes[i]; i++) {
        /* sanity check, utils_page_sizes should be ordered */
        if (i > 0) {
            assert(utils_page_sizes[i] < utils_page_sizes[i+1]);
        }
        if (size_bits == utils_page_sizes[i]) {
            return true;
        }
    }

    return false;
}

#endif /* _SEL4UTILS_PAGE_H_ */

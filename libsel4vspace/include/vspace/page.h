/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <utils/arith.h>

#include <vspace/arch/page.h>

#define SEL4_NUM_PAGE_SIZES ((int) ARRAY_SIZE(sel4_page_sizes))

static inline bool
sel4_valid_size_bits(size_t size_bits)
{
    for (int i = 0; i < SEL4_NUM_PAGE_SIZES && size_bits >= sel4_page_sizes[i]; i++) {
        /* sanity check, sel4_page_sizes should be ordered */
        if (i > 0) {
            assert(sel4_page_sizes[i - 1] < sel4_page_sizes[i]);
        }
        if (size_bits == sel4_page_sizes[i]) {
            return true;
        }
    }

    return false;
}

static inline int
sel4_page_size_bits_for_memory_region(size_t size_bytes) {
    int frame_size_index = 0;
    /* find the largest reasonable frame size */
    while (frame_size_index + 1 < SEL4_NUM_PAGE_SIZES) {
        if (size_bytes >> sel4_page_sizes[frame_size_index + 1] == 0) {
            break;
        }
        frame_size_index++;
    }
    return sel4_page_sizes[frame_size_index];
}


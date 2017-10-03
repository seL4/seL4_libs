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

#include <autoconf.h>
#include <assert.h>
#include <stdint.h>

#include <utils/util.h>

#include <vspace/page.h>
#include <sel4utils/arch/util.h>
#include <sel4utils/sel4_zf_logif.h>
#include <sel4utils/strerror.h>

/* Number of words in 64bits */
#define SEL4UTILS_64_WORDS (sizeof(uint64_t) / sizeof(seL4_Word))

/*
 * Get a 64bit value from the ipc buffer, starting from offset.
 * Assumes the low bits are in the first register and so on.
 *
 * @param offset the offser in the ipc buffer to start reading MRs at.
 * @return the 64bit value read from the ipc buffer.
 */
static inline uint64_t sel4utils_64_get_mr(seL4_Word offset)
{
    uint64_t result = seL4_GetMR(offset);

    if (SEL4UTILS_64_WORDS == 2) {
        result += (((uint64_t) seL4_GetMR(offset + 1)) << 32llu);
    }

    return result;
}

/*
 * Set the correct MRs for a 64bit value. SEL4UTILS_WORDS_IN_64 MRs will be used.
 *
 * @param offset offset in the IPC buffer to start setting MRs at.
 * @param value the value to set
 */
static inline void sel4utils_64_set_mr(seL4_Word offset, uint64_t value)
{
    seL4_SetMR(offset, (seL4_Word) value);
    if (SEL4UTILS_64_WORDS == 2) {
        seL4_SetMR(offset + 1, (seL4_Word) (value >> 32llu));
    }
    assert(sel4utils_64_get_mr(offset) == value);
}


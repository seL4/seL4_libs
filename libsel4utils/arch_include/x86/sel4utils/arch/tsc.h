/*
 * Copyright 2018, Data61
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

#include <stdint.h>
#include <string.h>
#include <simple/simple.h>
#include <utils/frequency.h>

// uses the simple extended bootinfo interface to attempt to determine the tsc frequency
// returns 0 on failure
static inline uint32_t
x86_get_tsc_freq_from_simple(simple_t *simple) {
    // the simple interface is a bit awkward and doesn't give us a way to just get the last 4
    // bytes so we have to define some scratch space to get what we want. use a char array instead
    // of a custom struct so we're not worrying about padding and aliasing etc
    char buf[sizeof(seL4_BootInfoHeader) + 4];
    ssize_t ret = simple_get_extended_bootinfo(simple, SEL4_BOOTINFO_HEADER_X86_TSC_FREQ, buf, sizeof(buf));
    if (ret == sizeof(buf)) {
        uint32_t freq;
        memcpy(&freq, buf + sizeof(seL4_BootInfoHeader), 4);
        // convert mhz to hz
        return freq * MHZ;
    }
    return 0;
}

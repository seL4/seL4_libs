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

#include <sel4/sel4.h>
#include <stddef.h>
#include <utils/util.h>

static UNUSED const char *register_names[] = {
    "pc",
    "sp",
    "cpsr",
    "r0",
    "r1",
    "r8",
    "r9",
    "r10",
    "r11",
    "r12",
    "r2",
    "r3",
    "r4",
    "r5",
    "r6",
    "r7",
    "lr"
};

/* assert that register_names correspond to seL4_UserContext */
compile_time_assert(pc_correct_position, offsetof(seL4_UserContext, pc)     == 0);
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext, sp)     == 1 *  sizeof(seL4_Word));
compile_time_assert(cpsr_correct_position, offsetof(seL4_UserContext, cpsr) == 2 *  sizeof(seL4_Word));
compile_time_assert(r0_correct_position, offsetof(seL4_UserContext, r0)     == 3 *  sizeof(seL4_Word));
compile_time_assert(r1_correct_position, offsetof(seL4_UserContext, r1)     == 4 *  sizeof(seL4_Word));
compile_time_assert(r8_correct_position, offsetof(seL4_UserContext, r8)     == 5 *  sizeof(seL4_Word));
compile_time_assert(r9_correct_position, offsetof(seL4_UserContext, r9)     == 6 *  sizeof(seL4_Word));
compile_time_assert(r10_correct_position, offsetof(seL4_UserContext, r10)   == 7 * sizeof(seL4_Word));
compile_time_assert(r11_correct_position, offsetof(seL4_UserContext, r11)   == 8 * sizeof(seL4_Word));
compile_time_assert(r12_correct_position, offsetof(seL4_UserContext, r12)   == 9 * sizeof(seL4_Word));
compile_time_assert(r2_correct_position, offsetof(seL4_UserContext, r2)     == 10 * sizeof(seL4_Word));
compile_time_assert(r3_correct_position, offsetof(seL4_UserContext, r3)     == 11 * sizeof(seL4_Word));
compile_time_assert(r4_correct_position, offsetof(seL4_UserContext, r4)     == 12 * sizeof(seL4_Word));
compile_time_assert(r5_correct_position, offsetof(seL4_UserContext, r5)     == 13 * sizeof(seL4_Word));
compile_time_assert(r6_correct_position, offsetof(seL4_UserContext, r6)     == 14 * sizeof(seL4_Word));
compile_time_assert(r7_correct_position, offsetof(seL4_UserContext, r7)     == 15 * sizeof(seL4_Word));
compile_time_assert(r14_correct_position, offsetof(seL4_UserContext, r14)   == 16 * sizeof(seL4_Word));


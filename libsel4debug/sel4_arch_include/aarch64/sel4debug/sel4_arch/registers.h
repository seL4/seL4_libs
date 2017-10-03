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
    "spsr",
    "x0",
    "x1",
    "x2",
    "x3",
    "x4",
    "x5",
    "x6",
    "x7",
    "x8",
    "x16",
    "x17",
    "x18",
    "x29",
    "x30",
    "x9",
    "x10",
    "x11",
    "x12",
    "x13",
    "x14",
    "x15",
    "x19",
    "x20",
    "x21",
    "x22",
    "x23",
    "x24",
    "x25",
    "x26",
    "x27",
    "x28"
};

/* assert that register_names correspond to seL4_UserContext */
compile_time_assert(pc_correct_position, offsetof(seL4_UserContext, pc)           == 0);
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext, sp)           == 1 *  sizeof(seL4_Word));
compile_time_assert(spsr_correct_position, offsetof(seL4_UserContext, spsr)       == 2 *  sizeof(seL4_Word));
compile_time_assert(x0_correct_position, offsetof(seL4_UserContext, x0)           == 3 *  sizeof(seL4_Word));
compile_time_assert(x1_correct_position, offsetof(seL4_UserContext, x1)           == 4 *  sizeof(seL4_Word));
compile_time_assert(x2_correct_position, offsetof(seL4_UserContext, x2)           == 5 *  sizeof(seL4_Word));
compile_time_assert(x3_correct_position, offsetof(seL4_UserContext, x3)           == 6 *  sizeof(seL4_Word));
compile_time_assert(x4_correct_position, offsetof(seL4_UserContext, x4)           == 7 * sizeof(seL4_Word));
compile_time_assert(x5_correct_position, offsetof(seL4_UserContext, x5)           == 8 * sizeof(seL4_Word));
compile_time_assert(x6_correct_position, offsetof(seL4_UserContext, x6)           == 9 * sizeof(seL4_Word));
compile_time_assert(x7_correct_position, offsetof(seL4_UserContext, x7)           == 10 * sizeof(seL4_Word));
compile_time_assert(x8_correct_position, offsetof(seL4_UserContext, x8)           == 11 * sizeof(seL4_Word));
compile_time_assert(x16_correct_position, offsetof(seL4_UserContext, x16)         == 12 * sizeof(seL4_Word));
compile_time_assert(x17_correct_position, offsetof(seL4_UserContext, x17)         == 13 * sizeof(seL4_Word));
compile_time_assert(x18_correct_position, offsetof(seL4_UserContext, x18)         == 14 * sizeof(seL4_Word));
compile_time_assert(x29_correct_position, offsetof(seL4_UserContext, x29)         == 15 * sizeof(seL4_Word));
compile_time_assert(x30_correct_position, offsetof(seL4_UserContext, x30)         == 16 * sizeof(seL4_Word));
compile_time_assert(x9_correct_position, offsetof(seL4_UserContext, x9)           == 17 * sizeof(seL4_Word));
compile_time_assert(x10_correct_position, offsetof(seL4_UserContext, x10)         == 18 * sizeof(seL4_Word));
compile_time_assert(x11_correct_position, offsetof(seL4_UserContext, x11)         == 19 *  sizeof(seL4_Word));
compile_time_assert(x12_correct_position, offsetof(seL4_UserContext, x12)         == 20 *  sizeof(seL4_Word));
compile_time_assert(x13_correct_position, offsetof(seL4_UserContext, x13)         == 21 *  sizeof(seL4_Word));
compile_time_assert(x14_correct_position, offsetof(seL4_UserContext, x14)         == 22 * sizeof(seL4_Word));
compile_time_assert(x15_correct_position, offsetof(seL4_UserContext, x15)         == 23 * sizeof(seL4_Word));
compile_time_assert(x19_correct_position, offsetof(seL4_UserContext, x19)         == 24 * sizeof(seL4_Word));
compile_time_assert(x20_correct_position, offsetof(seL4_UserContext, x20)         == 25 * sizeof(seL4_Word));
compile_time_assert(x21_correct_position, offsetof(seL4_UserContext, x21)         == 26 * sizeof(seL4_Word));
compile_time_assert(x22_correct_position, offsetof(seL4_UserContext, x22)         == 27 * sizeof(seL4_Word));
compile_time_assert(x23_correct_position, offsetof(seL4_UserContext, x23)         == 28 * sizeof(seL4_Word));
compile_time_assert(x24_correct_position, offsetof(seL4_UserContext, x24)         == 29 * sizeof(seL4_Word));
compile_time_assert(x25_correct_position, offsetof(seL4_UserContext, x25)         == 30 * sizeof(seL4_Word));
compile_time_assert(x26_correct_position, offsetof(seL4_UserContext, x26)         == 31 * sizeof(seL4_Word));
compile_time_assert(x27_correct_position, offsetof(seL4_UserContext, x27)         == 32 * sizeof(seL4_Word));
compile_time_assert(x28_correct_position, offsetof(seL4_UserContext, x28)         == 33 * sizeof(seL4_Word));


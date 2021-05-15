/*
 * Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <sel4/sel4.h>
#include <stddef.h>
#include <utils/util.h>

static UNUSED const char *register_names[] = {
    "pc",
    "ra",
    "sp",
    "gp",

    "s0",
    "s1",
    "s2",
    "s3",
    "s4",
    "s5",
    "s6",
    "s7",
    "s8",
    "s9",
    "s10",
    "s11",

    "a0",
    "a1",
    "a2",
    "a3",
    "a4",
    "a5",
    "a6",
    "a7",

    "t0",
    "t1",
    "t2",
    "t3",
    "t4",
    "t5",
    "t6",

    "tp",
};

/* assert that register_names correspond to seL4_UserContext */
compile_time_assert(register_names_correct_size, sizeof(register_names) == sizeof(seL4_UserContext));

compile_time_assert(pc_correct_position, offsetof(seL4_UserContext,  pc) ==  0 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  ra) ==  1 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  sp) ==  2 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  gp) ==  3 *  sizeof(seL4_Word));

compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  s0) ==  4 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  s1) ==  5 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  s2) ==  6 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  s3) ==  7 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  s4) ==  8 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  s5) ==  9 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  s6) == 10 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  s7) == 11 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  s8) == 12 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  s9) == 13 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext, s10) == 14 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext, s11) == 15 *  sizeof(seL4_Word));

compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  a0) == 16 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  a1) == 17 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  a2) == 18 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  a3) == 19 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  a4) == 20 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  a5) == 21 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  a6) == 22 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  a7) == 23 *  sizeof(seL4_Word));

compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  t0) == 24 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  t1) == 25 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  t2) == 26 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  t3) == 27 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  t4) == 28 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  t5) == 29 *  sizeof(seL4_Word));
compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  t6) == 30 *  sizeof(seL4_Word));

compile_time_assert(sp_correct_position, offsetof(seL4_UserContext,  tp) == 31 *  sizeof(seL4_Word));

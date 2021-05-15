/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sel4/sel4.h>
#include <stddef.h>
#include <utils/util.h>

static UNUSED const char *register_names[] = {
    "eip",
    "esp",
    "eflags",
    "eax",
    "ebx",
    "ecx",
    "edx",
    "esi",
    "edi",
    "ebp",
    "fs_base",
    "gs_base",
};
compile_time_assert(register_names_correct_size, sizeof(register_names) == sizeof(seL4_UserContext));

/* assert that register_names correspond to seL4_UserContext */
compile_time_assert(eip_correct_position, offsetof(seL4_UserContext, eip)           == 0);
compile_time_assert(esp_correct_position, offsetof(seL4_UserContext, esp)           == 1 *  sizeof(seL4_Word));
compile_time_assert(eflags_correct_position, offsetof(seL4_UserContext, eflags)     == 2 *  sizeof(seL4_Word));
compile_time_assert(eax_correct_position, offsetof(seL4_UserContext, eax)           == 3 *  sizeof(seL4_Word));
compile_time_assert(ebx_correct_position, offsetof(seL4_UserContext, ebx)           == 4 *  sizeof(seL4_Word));
compile_time_assert(ecx_correct_position, offsetof(seL4_UserContext, ecx)           == 5 *  sizeof(seL4_Word));
compile_time_assert(edx_correct_position, offsetof(seL4_UserContext, edx)           == 6 *  sizeof(seL4_Word));
compile_time_assert(esi_correct_position, offsetof(seL4_UserContext, esi)           == 7 * sizeof(seL4_Word));
compile_time_assert(edi_correct_position, offsetof(seL4_UserContext, edi)           == 8 * sizeof(seL4_Word));
compile_time_assert(ebp_correct_position, offsetof(seL4_UserContext, ebp)           == 9 * sizeof(seL4_Word));
compile_time_assert(fs_base_correct_position, offsetof(seL4_UserContext, fs_base)        == 10 * sizeof(seL4_Word));
compile_time_assert(gs_base_correct_position, offsetof(seL4_UserContext, gs_base)        == 11 * sizeof(seL4_Word));


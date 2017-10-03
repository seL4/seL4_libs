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
    "tls_base",
    "fs",
    "gs",
};

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
compile_time_assert(tls_base_correct_position, offsetof(seL4_UserContext, tls_base) == 10 * sizeof(seL4_Word));
compile_time_assert(fs_correct_position, offsetof(seL4_UserContext, fs)             == 11 * sizeof(seL4_Word));
compile_time_assert(gs_correct_position, offsetof(seL4_UserContext, gs)             == 12 * sizeof(seL4_Word));


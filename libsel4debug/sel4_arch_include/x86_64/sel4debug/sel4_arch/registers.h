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
    "rip",
    "rsp",
    "rflags",
    "rax",
    "rbx",
    "rcx",
    "rdx",
    "rsi",
    "rdi",
    "rbp",
    "r8",
    "r9",
    "r10",
    "r11",
    "r12",
    "r13",
    "r14",
    "r15",
    "fs_base",
    "gs_base",
};
compile_time_assert(register_names_correct_size, sizeof(register_names) == sizeof(seL4_UserContext));

/* assert that register_names correspond to seL4_UserContext */
compile_time_assert(eip_correct_position, offsetof(seL4_UserContext, rip)           == 0);
compile_time_assert(esp_correct_position, offsetof(seL4_UserContext, rsp)           == 1 *  sizeof(seL4_Word));
compile_time_assert(eflags_correct_position, offsetof(seL4_UserContext, rflags)     == 2 *  sizeof(seL4_Word));
compile_time_assert(eax_correct_position, offsetof(seL4_UserContext, rax)           == 3 *  sizeof(seL4_Word));
compile_time_assert(ebx_correct_position, offsetof(seL4_UserContext, rbx)           == 4 *  sizeof(seL4_Word));
compile_time_assert(ecx_correct_position, offsetof(seL4_UserContext, rcx)           == 5 *  sizeof(seL4_Word));
compile_time_assert(edx_correct_position, offsetof(seL4_UserContext, rdx)           == 6 *  sizeof(seL4_Word));
compile_time_assert(esi_correct_position, offsetof(seL4_UserContext, rsi)           == 7 * sizeof(seL4_Word));
compile_time_assert(edi_correct_position, offsetof(seL4_UserContext, rdi)           == 8 * sizeof(seL4_Word));
compile_time_assert(ebp_correct_position, offsetof(seL4_UserContext, rbp)           == 9 * sizeof(seL4_Word));
compile_time_assert(ebp_correct_position, offsetof(seL4_UserContext, r8)            == 10 * sizeof(seL4_Word));
compile_time_assert(ebp_correct_position, offsetof(seL4_UserContext, r9)            == 11 * sizeof(seL4_Word));
compile_time_assert(ebp_correct_position, offsetof(seL4_UserContext, r10)           == 12 * sizeof(seL4_Word));
compile_time_assert(ebp_correct_position, offsetof(seL4_UserContext, r11)           == 13 * sizeof(seL4_Word));
compile_time_assert(ebp_correct_position, offsetof(seL4_UserContext, r12)           == 14 * sizeof(seL4_Word));
compile_time_assert(ebp_correct_position, offsetof(seL4_UserContext, r13)           == 15 * sizeof(seL4_Word));
compile_time_assert(ebp_correct_position, offsetof(seL4_UserContext, r14)           == 16 * sizeof(seL4_Word));
compile_time_assert(ebp_correct_position, offsetof(seL4_UserContext, r15)           == 17 * sizeof(seL4_Word));
compile_time_assert(fs_base_correct_position, offsetof(seL4_UserContext, fs_base)   == 18 * sizeof(seL4_Word));
compile_time_assert(gs_base_correct_position, offsetof(seL4_UserContext, gs_base)   == 19 * sizeof(seL4_Word));

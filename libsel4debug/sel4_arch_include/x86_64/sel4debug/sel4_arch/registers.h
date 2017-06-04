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
    "tls_base",
};

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
compile_time_assert(tls_base_correct_position, offsetof(seL4_UserContext, tls_base) == 18 * sizeof(seL4_Word));

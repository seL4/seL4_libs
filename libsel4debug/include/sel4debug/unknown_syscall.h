/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sel4/sel4.h>

void debug_unknown_syscall_message(int (*printfn)(const char *format, ...),
                                   seL4_Word* mrs);


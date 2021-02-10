/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sel4/sel4.h>

#include <simple/simple.h>

void simple_default_init_bootinfo(simple_t *simple, seL4_BootInfo *bi);
void simple_default_init_arch_simple(arch_simple_t *simple, void *data);


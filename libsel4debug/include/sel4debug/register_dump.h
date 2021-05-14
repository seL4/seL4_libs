/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sel4debug/arch/registers.h>

void sel4debug_dump_registers(seL4_CPtr tcb);
void sel4debug_dump_registers_prefix(seL4_CPtr tcb, char *prefix);

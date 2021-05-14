/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sel4/sel4.h>
#include <utils/util.h>
#include <platsupport/io.h>

/* Functions provided from platform layer to arch code. */
int
__plat_serial_init(ps_io_ops_t* io_ops);
void
__plat_putchar(int c);
int
__plat_getchar(void);


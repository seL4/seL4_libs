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
#include <utils/util.h>
#include <platsupport/io.h>

/* Functions provided from platform layer to arch code. */
int
__plat_serial_init(ps_io_ops_t* io_ops);
void
__plat_putchar(int c);
int
__plat_getchar(void);


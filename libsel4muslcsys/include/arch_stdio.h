/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <autoconf.h>
#include <sel4muslcsys/gen_config.h>
#include <stddef.h>

/*
 * Each architecture implements the following simple putchar function, usually
 * setup to output to serial.
 */
void __arch_putchar(int c) __attribute__((noinline));
size_t __arch_write(char *data, size_t count) __attribute__((noinline));

/*
 * Each architecture implements the following simple getchar function, usually
 * setup to receive from serial.
 */
int __arch_getchar(void);


typedef size_t (*write_buf_fn)(void *data, size_t count);
/*
 * Register a function to be called when sys_writev is called on either
 * stdout or stderr.  This will return the existing function that is registered.
 * Calling this function with NULL is valid and will result in no function being
 * called, but writev still returning the number of characters written.  This is
 * similar to piping to /dev/null.
 */
write_buf_fn sel4muslcsys_register_stdio_write_fn(write_buf_fn write_fn);

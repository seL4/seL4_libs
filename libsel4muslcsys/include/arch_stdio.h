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

#include <autoconf.h>
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

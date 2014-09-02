/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _LIBSEL4DEBUG_SERIAL_H_
#define _LIBSEL4DEBUG_SERIAL_H_

/* Initialise the debug serial driver. Without setting this up you will not be
 * able to print via debug_putchar on a non-debug kernel. Initialisation data
 * is platform specific. Returns 0 on success.
 */
int debug_plat_serial_init(void *data);

/* Print a character to the initialised serial device. Returns 0 on success. */
int debug_putchar(int c);

#endif

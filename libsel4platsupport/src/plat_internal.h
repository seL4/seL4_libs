/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLAT_INTERNAL_H__
#define __PLAT_INTERNAL_H__

#include <sel4/sel4.h>
#include <sel4utils/util.h>
#include <platsupport/io.h>

/* Functions provided from platform layer to arch code. */
void
__plat_serial_input_init_IRQ(void);
int
__plat_serial_init(ps_io_ops_t* io_ops);
void
__plat_putchar(int c);
int
__plat_getchar(void);

#endif /* __PLAT_INTERNAL_H__ */


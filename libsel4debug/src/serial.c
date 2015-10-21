/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <autoconf.h>
#include <sel4debug/serial.h>
#include <sel4/sel4.h>

/* Implemented in plat/.../serial.c. */
int debug_plat_putchar(int c);

int debug_putchar(int c)
{
    if (c == '\n') {
        debug_putchar('\r');
    }
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugPutChar(c);
    return 0;
#else
    return debug_plat_putchar(c);
#endif
}

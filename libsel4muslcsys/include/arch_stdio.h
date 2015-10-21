/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _ARCH_STDIO_H_
#define _ARCH_STDIO_H_

#include <autoconf.h>

/*
 * Each architecture implements the following simple putchar function, usually
 * setup to output to serial.
 */
void __arch_putchar(int c) __attribute__((noinline));

#ifdef CONFIG_LIB_SEL4_MUSLC_SYS_ARCH_PUTCHAR_WEAK
    void __arch_putchar(int c) __attribute__((weak));
#endif

/*
 * Each architecture implements the following simple getchar function, usually
 * setup to receive from serial.
 */
int __arch_getchar(void);

#endif /* _ARCH_STDIO_H_ */

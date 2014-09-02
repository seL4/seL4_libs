/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _LIBSEL4DEBUG_UNKNOWN_SYSCALL_H_
#define _LIBSEL4DEBUG_UNKNOWN_SYSCALL_H_

#include <sel4/sel4.h>

void debug_unknown_syscall_message(int (*printfn)(const char *format, ...),
                                   seL4_Word* mrs);

#endif

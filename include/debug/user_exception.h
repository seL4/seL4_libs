/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _LIBSEL4DEBUG_USER_EXCEPTION_H_
#define _LIBSEL4DEBUG_USER_EXCEPTION_H_

#include <sel4/sel4.h>

void debug_user_exception_message(int (*printfn)(const char *format, ...),
                                  seL4_Word* mrs);

#endif

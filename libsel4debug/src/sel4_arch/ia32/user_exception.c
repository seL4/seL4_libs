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

#include <sel4/sel4.h>
#include <sel4debug/user_exception.h>

void debug_user_exception_message(int (*printfn)(const char *format, ...),
                                  seL4_Word* mrs)
{
    /* See section 5.2.3 of the manual. */
    printfn(" EIP = %p\n"
            " ESP = %p\n"
            " EFLAGS = %p\n"
            " exception number = %p\n"
            " exception code = %p\n",
            (void*)mrs[0], (void*)mrs[1], (void*)mrs[2], (void*)mrs[3],
            (void*)mrs[4]);
}

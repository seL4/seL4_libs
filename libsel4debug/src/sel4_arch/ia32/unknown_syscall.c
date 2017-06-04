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
#include <sel4debug/unknown_syscall.h>

void debug_unknown_syscall_message(int (*printfn)(const char *format, ...),
                                   seL4_Word* mrs)
{
    /* See section 5.2.2 of the manual. */
    printfn(" EAX = %p\n"
            " EBX = %p\n"
            " ECX = %p\n"
            " EDX = %p\n"
            " ESI = %p\n"
            " EDI = %p\n"
            " EBP = %p\n"
            " EIP = %p\n"
            " ESP = %p\n"
            " EFLAGS = %p\n"
            " syscall = %p\n",
            (void*)mrs[0], (void*)mrs[1], (void*)mrs[2], (void*)mrs[3],
            (void*)mrs[4], (void*)mrs[5], (void*)mrs[6], (void*)mrs[7],
            (void*)mrs[8], (void*)mrs[9], (void*)mrs[10]);
}

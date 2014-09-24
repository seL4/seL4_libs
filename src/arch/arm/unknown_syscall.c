/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <sel4/sel4.h>
#include <sel4debug/unknown_syscall.h>

void debug_unknown_syscall_message(int (*printfn)(const char *format, ...),
                                   seL4_Word* mrs)
{
    /* See section 5.2.2 of the manual. */
    for (unsigned int i = 0; i < 8; i++) {
        printfn(" R%u = %p\n", i, (void*)mrs[i]);
    }
    printfn(" PC = %p\n"
            " SP = %p\n"
            " LR = %p\n"
            " CPSR = %p\n"
            " syscall = %p\n",
            (void*)mrs[8], (void*)mrs[9], (void*)mrs[10], (void*)mrs[11],
            (void*)mrs[12]);
}

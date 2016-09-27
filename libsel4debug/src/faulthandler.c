/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <assert.h>
#include <sel4/sel4.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sel4debug/faults.h>
#include <sel4debug/unknown_syscall.h>
#include <sel4debug/user_exception.h>

/* Maximum number of message registers used in any fault transmission. */
#define USED_MRS 13

int debug_fault_handler(seL4_CPtr faultep,
                        const char * (*senders)(seL4_Word badge))
{
    char *sender_name = NULL;

    /* If we weren't passed a translation function, make sender_name a buffer
     * large enough to handle sprintf-ing a pointer into it.
     */
    if (senders == NULL) {
        sender_name = (char*)malloc(sizeof(uintptr_t) * 2 + strlen("0x") + 1);
        if (sender_name == NULL) {
            /* Out of memory */
            return -1;
        }
    }

    while (1) {
        seL4_Word sender;

        /* Wait for a fault. */
        seL4_MessageInfo_t info = seL4_Recv(faultep, &sender);

        /* Get the fault type (see section 5.2 of the manual). */
        seL4_Word label = seL4_MessageInfo_get_label(info);

        seL4_Word length = seL4_MessageInfo_get_length(info);

        /* Save the message registers before doing anything else. This is under
         * the assumption printf() or senders() may clobber them.
         */
        seL4_Word mrs[USED_MRS];
        for (unsigned int i = 0; i < USED_MRS; i++) {
            mrs[i] = seL4_GetMR(i);
        }

        /* Retrieve the friendly name of the thread if its available. */
        if (senders == NULL) {
            sprintf(sender_name, "%p", (void*)sender);
        } else {
            sender_name = (char*)senders(sender);
        }

        switch (label) {

        case seL4_CapFault: {
            /* See section 5.2.1 of the manual. */
            assert(length > 4);
            printf("debug: Cap fault in %s phase from %s\n"
                   " PC   = %p\n"
                   " CPtr = %p\n",
                   mrs[2] == 1 ? "receive" : "send", sender_name,
                   (void*)mrs[0], (void*)mrs[1]);
            /* TODO: Failure description. */
            break;
        }

        case seL4_UnknownSyscall: {
            /* See section 5.2.2 of the manual. */
            printf("debug: Unknown syscall from %s\n", sender_name);
            debug_unknown_syscall_message(printf, mrs);
            break;
        }

        case seL4_UserException: {
            /* See section 5.2.3 of the manual. */
            printf("debug: User exception from %s\n", sender_name);
            debug_user_exception_message(printf, mrs);
            break;
        }

        case seL4_VMFault: {
            /* See section 5.2.4 of the manual. */
            assert(length == 4);
            printf("debug: Virtual memory %s fault from %s\n"
                   " PC                    = %p\n"
                   " Fault address         = %p\n"
                   " Fault status register = %p\n",
                   mrs[2] == 1 ? "instruction" : "data", sender_name,
                   (void*)mrs[0], (void*)mrs[1], (void*)mrs[3]);
            /* TODO: Translate the FSR nicely for the user. */
            break;
        }

        default: {
            printf("debug: Unexpected fault %p from %s\n", (void*)label,
                   sender_name);
            for (unsigned int i = 0; i < length && i < USED_MRS; i++) {
                printf(" mr%u = %p\n", i, (void*)mrs[i]);
            }
            if (length > USED_MRS) {
                printf("  further %zu registers not shown\n",
                       length - USED_MRS);
            }
        }
        }
    }

    assert(!"unreachable");
    return 0;
}

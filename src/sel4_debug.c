/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdio.h> /* For fprintf() */
#include <stdlib.h> /* For abort() */
#include <sel4utils/sel4_debug.h>

char *sel4_errlist[] = {
    "seL4_NoError",
    "seL4_InvalidArgument",
    "seL4_InvalidCapability",
    "seL4_IllegalOperation",
    "seL4_RangeError",
    "seL4_AlignmentError",
    "seL4_FailedLookup",
    "seL4_TruncatedMessage",
    "seL4_DeleteFirst",
    "seL4_RevokeFirst",
    "seL4_NotEnoughMemory",
    NULL
};

void
__sel4_error(int sel4_error, const char *file,
             const char *function, int line, char * str)
{
    fprintf(stderr, "seL4 Error: %s, function %s, file %s, line %d: %s\n",
            sel4_errlist[sel4_error],
            function, file, line, str);
    abort();
}

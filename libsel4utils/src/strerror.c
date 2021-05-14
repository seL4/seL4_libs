/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h> /* For fprintf() */
#include <stdlib.h> /* For abort() */
#include <assert.h>
#include <sel4/sel4.h>
#include <sel4utils/strerror.h>

#define _PRIV_SEL4_FAULTLIST_UNKNOWN_IDX (seL4_Fault_UserException + 1)
#ifdef CONFIG_ARM_HYPERVISOR_SUPPORT
#define _PRIV_SEL4_FAULTLIST_MAX_IDX     (seL4_Fault_VCPUFault)
#else
#define _PRIV_SEL4_FAULTLIST_MAX_IDX     (seL4_Fault_VMFault)
#endif

char *sel4_errlist[] = {
    [seL4_NoError] = "seL4_NoError",
    [seL4_InvalidArgument] = "seL4_InvalidArgument",
    [seL4_InvalidCapability] = "seL4_InvalidCapability",
    [seL4_IllegalOperation] = "seL4_IllegalOperation",
    [seL4_RangeError] = "seL4_RangeError",
    [seL4_AlignmentError] = "seL4_AlignmentError",
    [seL4_FailedLookup] = "seL4_FailedLookup",
    [seL4_TruncatedMessage] = "seL4_TruncatedMessage",
    [seL4_DeleteFirst] = "seL4_DeleteFirst",
    [seL4_RevokeFirst] = "seL4_RevokeFirst",
    [seL4_NotEnoughMemory] = "seL4_NotEnoughMemory",
    NULL
};

char *sel4_faultlist[] = {
    [seL4_Fault_NullFault] = "seL4_Fault_NullFault",
    [seL4_Fault_CapFault] = "seL4_Fault_CapFault",
    [seL4_Fault_UnknownSyscall] = "seL4_Fault_UnknownSyscall",
    [seL4_Fault_UserException] = "seL4_Fault_UserException",
    [_PRIV_SEL4_FAULTLIST_UNKNOWN_IDX] = "Unknown Fault",
    [seL4_Fault_VMFault] = "seL4_Fault_VMFault",
#ifdef CONFIG_ARM_HYPERVISOR_SUPPORT
    [seL4_Fault_VGICMaintenance] = "seL4_Fault_VGICMaintenance",
    [seL4_Fault_VCPUFault] = "seL4_Fault_VCPUFault"
#endif
};

const char *
sel4_strerror(int errcode)
{
    assert(errcode < sizeof(sel4_errlist) / sizeof(*sel4_errlist) - 1);
    return sel4_errlist[errcode];
}

void
__sel4_error(int sel4_error, const char *file,
             const char *function, int line, char * str)
{
    fprintf(stderr, "seL4 Error: %s, function %s, file %s, line %d: %s\n",
            sel4_errlist[sel4_error],
            function, file, line, str);
    abort();
}

const char *
sel4_strfault(int faultlabel)
{
    if (faultlabel > _PRIV_SEL4_FAULTLIST_MAX_IDX || faultlabel == _PRIV_SEL4_FAULTLIST_UNKNOWN_IDX
	|| faultlabel < seL4_Fault_NullFault) {
        return sel4_faultlist[_PRIV_SEL4_FAULTLIST_UNKNOWN_IDX];
    }

    return sel4_faultlist[faultlabel];
}

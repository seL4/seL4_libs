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
#include <stdio.h>
#include <stdlib.h>
#include <utils/util.h>

seL4_BootInfo *platsupport_get_bootinfo(void)
{
    /* bootinfo was set as an environment variable in _sel4_start */
    char *bi_string = getenv("bootinfo");
    if (!bi_string) {
        ZF_LOGE("Attempted %s in an environment without bootinfo.", __FUNCTION__);
        return NULL;
    }
    void *bi;
    if (sscanf(bi_string, "%p", &bi) != 1) {
        ZF_LOGE("bootinfo environment value '%s' was not valid.", bi_string);
        return NULL;
    }
    seL4_BootInfo *bootinfo = (seL4_BootInfo*)bi;
    /* Save the address of the IPC buffer for seL4_GetIPCBuffer on IA32. */
    seL4_SetUserData((seL4_Word)bootinfo->ipcBuffer);
    return bootinfo;
}

seL4_CPtr sel4ps_get_seL4_CapInitThreadTCB(void)
{
    return seL4_CapInitThreadTCB;
}

/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sel4/sel4.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/util.h>
#include <sel4runtime.h>

seL4_BootInfo *platsupport_get_bootinfo(void)
{
    /* bootinfo was set as an environment variable in _sel4_start */
    seL4_BootInfo *bootinfo = sel4runtime_bootinfo();
    if (!bootinfo) {
        ZF_LOGE("Attempted %s in an environment without bootinfo.", __FUNCTION__);
        return NULL;
    }
    return bootinfo;
}

seL4_CPtr sel4ps_get_seL4_CapInitThreadTCB(void)
{
    return seL4_CapInitThreadTCB;
}

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <sel4debug/debug.h>
#include <stdlib.h> /* To squash errors in functions.h. */
#include <sel4/types.h>
#include <sel4/arch/functions.h>
#include <stdarg.h>
#include <stdio.h>

int
debug_safe_printf(const char *format, ...)
{
    seL4_IPCBuffer shadow_buffer; /* XXX: Should we avoid allocating
                                   * this large struct on the stack?
                                   */

    /* Assume that the standard printf may clobber the IPC buffer, so save it
     * before. It would be nice to just memcpy this, but we don't have the
     * buffer address on IA32.
     */
    shadow_buffer.tag = seL4_GetTag();
    for (int i = 0; i < seL4_MsgMaxLength; ++i) {
        shadow_buffer.msg[i] = seL4_GetMR(i);
    }
    shadow_buffer.userData = seL4_GetUserData();
    for (int i = 0; i < seL4_MsgMaxExtraCaps; ++i) {
        shadow_buffer.caps_or_badges[i] = (seL4_Word)seL4_GetCap(i);
    }
    seL4_GetCapReceivePath(&shadow_buffer.receiveCNode,
                           &shadow_buffer.receiveIndex, &shadow_buffer.receiveDepth);

    /* Copied from printf: */
    int ret;
    va_list ap;

    va_start(ap, format);
    ret = vfprintf(stdout, format, ap);
    va_end(ap);

    /* Now restore the IPC buffer. */
    seL4_SetTag(shadow_buffer.tag);
    for (int i = 0; i < seL4_MsgMaxLength; ++i) {
        seL4_SetMR(i, shadow_buffer.msg[i]);
    }
    seL4_SetUserData(shadow_buffer.userData);
    for (int i = 0; i < seL4_MsgMaxExtraCaps; ++i) {
        seL4_SetCap(i, (seL4_CPtr)shadow_buffer.caps_or_badges[i]);
    }
    seL4_SetCapReceivePath(shadow_buffer.receiveCNode,
                           shadow_buffer.receiveIndex, shadow_buffer.receiveDepth);

    return ret;
}

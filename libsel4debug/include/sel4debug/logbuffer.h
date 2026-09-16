/*
 * Copyright 2020, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#pragma once

/* Utilities for working with the debug log buffer */

#include <autoconf.h>
#include <sel4/sel4.h>
#include <sel4/log.h>
#include <sel4/syscalls.h>
#include <utils/base64.h>
#include <vka/object.h>
#include <vspace/vspace.h>

/*
 * Allocates and maps a new kernel log buffer.
 *
 * The large frame used for the kernel log buffer is placed in the frame
 * argument and can be mapped into other address spaces.
 */
static int debug_log_buffer_init(vka_t *vka, vspace_t *vspace, vka_object_t *frame, seL4_LogBuffer *buffer)
{
#ifdef CONFIG_KERNEL_DEBUG_LOG_BUFFER
    int err;
    void *volatile vaddr;

    err = vka_alloc_frame(vka, seL4_LogBufferBits, frame);
    if (err != 0) {
        return err;
    }

    err = seL4_BenchmarkSetLogBuffer(frame->cptr);
    if (err != 0) {
        ZF_LOGE("Failed to set debug log buffer frame using cap %lu.\n", frame->cptr);
        return err;
    }

    vaddr = vspace_map_pages(vspace, &frame->cptr, NULL, seL4_AllRights, 1, seL4_LogBufferBits, 1);
    if (vaddr == NULL) {
        ZF_LOGE("Failed to map log buffer into VMM addrspace.\n");
        return -1;
    }

    *buffer = seL4_LogBuffer_new(vaddr);

    ZF_LOGD("Log buffer (cptr %lu) mapped to vaddr %p.\n", frame->cptr, vaddr);

    return seL4_NoError;
#else
    *buffer = seL4_LogBuffer_new(NULL);
    return seL4_NoError;
#endif
}

/*
 * Reset the debug log buffer.
 *
 * All new events will be placed at the start of the buffer.
 */
static int debug_log_buffer_reset(seL4_LogBuffer *buffer)
{
    seL4_LogBuffer_reset(buffer);
#ifdef CONFIG_KERNEL_DEBUG_LOG_BUFFER
    return seL4_BenchmarkResetLog();
#else
    return seL4_NoError;
#endif
}

/*
 * Finalise the debug log buffer.
 *
 * All new events will be placed at the start of the buffer.
 */
static void debug_log_buffer_finalise(seL4_LogBuffer *buffer)
{
#ifdef CONFIG_KERNEL_DEBUG_LOG_BUFFER
    seL4_LogBuffer_setSize(buffer, seL4_BenchmarkFinalizeLog());
#else
    seL4_LogBuffer_reset(buffer);
#endif
}

/*
 * Dump the debug log to the given output.
 *
 * This will also finalise the kernel log buffer, stopping subsequent
 * output.
 */
int debug_log_buffer_dump_cbor64(seL4_LogBuffer *buffer, base64_t *streamer);

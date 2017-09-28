/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sel4/sel4.h>
#include <sel4utils/util.h>
#include <simple/simple.h>

#define VCHAN_BUF_SIZE PAGE_SIZE_4K
#define NUM_BUFFERS 2

typedef struct vchan_buf {
    int owner;
    char sync_data[VCHAN_BUF_SIZE];
    int filled;
    int read_pos, write_pos;
} vchan_buf_t;

/*
    Handles managing of packets, storing packets in shared mem,
        copying in memory and reading from memory for sync comms
*/
typedef struct vchan_shared_mem {
    int alloced;
    vchan_buf_t bufs[2];
} vchan_shared_mem_t;

typedef struct vchan_shared_mem_headers {
    vchan_shared_mem_t shared_buffers[NUM_BUFFERS];
    int token;
} vchan_headers_t;


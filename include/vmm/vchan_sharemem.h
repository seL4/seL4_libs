/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef _VCHAN_SHARED_MEM
#define _VCHAN_SHARED_MEM

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

#endif

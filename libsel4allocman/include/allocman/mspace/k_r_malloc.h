/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdlib.h>

/* A K&R malloc style allocation that can be 'put in a box' as it were. */

typedef union k_r_malloc_header {
    struct {
        union k_r_malloc_header *ptr;
        size_t size;
    } s;
    /* Force alignment */
    long long x;
} k_r_malloc_header_t;

typedef struct mspace_k_r_malloc {
    k_r_malloc_header_t base;
    k_r_malloc_header_t *freep;
    size_t cookie;
    k_r_malloc_header_t *(*morecore)(size_t cookie, struct mspace_k_r_malloc *k_r_malloc, size_t new_units);
} mspace_k_r_malloc_t;

void mspace_k_r_malloc_init(mspace_k_r_malloc_t *k_r_malloc, size_t cookie, k_r_malloc_header_t * (*morecore)(size_t cookie, mspace_k_r_malloc_t *k_r_malloc, size_t new_units));
void *mspace_k_r_malloc_alloc(mspace_k_r_malloc_t *k_r_malloc, size_t nbytes);
void mspace_k_r_malloc_free(mspace_k_r_malloc_t *k_r_malloc, void *ap);


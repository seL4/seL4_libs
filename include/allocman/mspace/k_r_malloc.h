/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _ALLOCMAN_MSPACE_K_R_MALLOC_H_
#define _ALLOCMAN_MSPACE_K_R_MALLOC_H_

#include <stdint.h>

/* A K&R malloc style allocation that can be 'put in a box' as it were. */

typedef union k_r_malloc_header {
    struct {
        union k_r_malloc_header *ptr;
        uint32_t size;
    } s;
    /* Force alignment */
    long long x;
} k_r_malloc_header_t;

typedef struct mspace_k_r_malloc {
    k_r_malloc_header_t base;
    k_r_malloc_header_t *freep;
    uint32_t cookie;
    k_r_malloc_header_t *(*morecore)(uint32_t cookie, struct mspace_k_r_malloc *k_r_malloc, uint32_t new_units);
} mspace_k_r_malloc_t;

void mspace_k_r_malloc_init(mspace_k_r_malloc_t *k_r_malloc, uint32_t cookie, k_r_malloc_header_t * (*morecore)(uint32_t cookie, mspace_k_r_malloc_t *k_r_malloc, uint32_t new_units));
void *mspace_k_r_malloc_alloc(mspace_k_r_malloc_t *k_r_malloc, uint32_t nbytes);
void mspace_k_r_malloc_free(mspace_k_r_malloc_t *k_r_malloc, void *ap);

#endif

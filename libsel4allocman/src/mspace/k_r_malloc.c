/* @LICENSE(CUSTOM) */

/* An implementation of malloc as described in the K&R C programming book */

#include <allocman/mspace/k_r_malloc.h>
#include <stddef.h> /* For NULL */
#include <stdlib.h>
#include <string.h> /* For memcpy */

void mspace_k_r_malloc_init(mspace_k_r_malloc_t *k_r_malloc, size_t cookie, k_r_malloc_header_t * (*morecore)(size_t cookie, mspace_k_r_malloc_t *k_r_malloc, size_t new_units))
{
    k_r_malloc->freep = NULL;
    k_r_malloc->cookie = cookie;
    k_r_malloc->morecore = morecore;
}

void *mspace_k_r_malloc_alloc(mspace_k_r_malloc_t *k_r_malloc, size_t nbytes)
{
    k_r_malloc_header_t *p, *prevp;
    size_t nunits;
    nunits = (nbytes + sizeof(k_r_malloc_header_t) - 1) / sizeof(k_r_malloc_header_t) + 1;
    if ((prevp = k_r_malloc->freep) == NULL) {	/* no free list yet */
        k_r_malloc->base.s.ptr = k_r_malloc->freep = prevp = &k_r_malloc->base;
        k_r_malloc->base.s.size = 0;
    }
    for (p = prevp->s.ptr;; prevp = p, p = p->s.ptr) {
        if (p->s.size >= nunits) {	/* big enough */
            if (p->s.size == nunits) {	/* exactly */
                prevp->s.ptr = p->s.ptr;
            } else {	/* allocate tail end */
                p->s.size -= nunits;
                p += p->s.size;
                p->s.size = nunits;
            }
            k_r_malloc->freep = prevp;
            return (void *) (p + 1);
        }
        if (p == k_r_malloc->freep) {	/* wrapped around free list */
            if ((p = (k_r_malloc_header_t *) k_r_malloc->morecore(k_r_malloc->cookie, k_r_malloc, nunits)) == NULL) {
                return NULL;	/* none left */
            } else {
                /* Put the thing in */
                p->s.size = nunits;
                mspace_k_r_malloc_free(k_r_malloc, p + 1);
                p = k_r_malloc->freep;
            }
        }
    }
}

void mspace_k_r_malloc_free(mspace_k_r_malloc_t *k_r_malloc, void *ap)
{
    k_r_malloc_header_t *bp, *p;

    if (ap == NULL) {
        return;
    }

    bp = (k_r_malloc_header_t *) ap - 1;	/* point to block header */
    for (p = k_r_malloc->freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
        if (p >= p->s.ptr && (bp > p || bp < p->s.ptr)) {
            break;    /* freed block at start or end of area */
        }

    if (bp + bp->s.size == p->s.ptr) {	/* join to upper nbr */
        bp->s.size += p->s.ptr->s.size;
        bp->s.ptr = p->s.ptr->s.ptr;
    } else {
        bp->s.ptr = p->s.ptr;
    }

    if (p + p->s.size == bp) {	/* join to lower nbr */
        p->s.size += bp->s.size;
        p->s.ptr = bp->s.ptr;
    } else {
        p->s.ptr = bp;
    }

    k_r_malloc->freep = p;

}

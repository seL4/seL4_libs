/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <assert.h>
#include <sel4/sel4.h>
#include <stdint.h>
#include <stdlib.h>
#include <vka/cspacepath_t.h>
#include <vka/null-vka.h>
#include <vka/vka.h>

static int cspace_alloc(void *data, seL4_CPtr *res) {
    return -1;
}

static void cspace_make_path(void *data, seL4_CPtr slot, cspacepath_t *res) {
}

static void cspace_free(void *data, seL4_CPtr slot) {
}

static int utspace_alloc(void *data, const cspacepath_t *dest, seL4_Word type,
        seL4_Word size_bits, uint32_t *res) {
    return -1;
}

static void utspace_free(void *data, seL4_Word type, seL4_Word size_bits,
        uint32_t target) {
}

void vka_init_nullvka(vka_t *vka) {
    assert(vka != NULL);
    vka->data = NULL; /* not required */
    vka->cspace_alloc = cspace_alloc;
    vka->cspace_make_path = cspace_make_path;
    vka->utspace_alloc = utspace_alloc;
    vka->cspace_free = cspace_free;
    vka->utspace_free = utspace_free;
}

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _VKA_NULL_VKA_H_
#define _VKA_NULL_VKA_H_

#include <vka/vka.h>

/* Initialise an allocator that always fails. This can be useful when you just
 * need to pacify something that expects VKA.
 */
void vka_init_nullvka(vka_t *vka);

#endif

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _SIMPLE_STABLE_H_
#define _SIMPLE_STABLE_H_

#include <sel4/sel4.h>

#include <simple/simple.h>

/* Backwards compatbility reasons means this function should stay here
 * One day someone may remove it, use simple_stable_init_bootinfo from now on
 */
void simple_cdt_init_bootinfo(simple_t *simple, seL4_BootInfo *bi);

void simple_stable_init_bootinfo(simple_t *simple, seL4_BootInfo *bi);

/* Some of the internal functions are useful without construction of a simple.
 * we expose them here to avoid logic duplication. For type siginature reasons
 * bootinfo is declared as a void pointer, this is to match how the function
 * is used internally */
seL4_Error simple_stable_get_frame_cap(void *bootinfo, void *paddr, int size_bits, cspacepath_t *path);
void *simple_stable_get_frame_info(void *data, void *paddr, int size_bits, seL4_CPtr *frame_cap, seL4_Word *ut_offset);

#endif /* _SIMPLE_STABLE_H_ */

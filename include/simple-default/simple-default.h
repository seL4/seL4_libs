/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _SIMPLE_DEFAULT_H_
#define _SIMPLE_DEFAULT_H_

#include <sel4/sel4.h>

#include <simple/simple.h>

void simple_default_init_bootinfo(simple_t *simple, seL4_BootInfo *bi);

#endif /* _SIMPLE_DEFAULT_H_ */

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _LIBSEL4DEBUG_REGISTER_DUMP_H_
#define _LIBSEL4DEBUG_REGISTER_DUMP_H_

#include <sel4debug/arch/registers.h>

void sel4debug_dump_registers(seL4_CPtr tcb);

#endif /* !_LIBSEL4DEBUG_REGISTER_DUMP_H_ */

/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#pragma once

#include <stdbool.h>
#include <sel4/sel4.h>
#include <simple/simple.h>
#include <vka/vka.h>

bool simple_is_untyped_cap(simple_t *simple, seL4_CPtr pos);

/* Returns the capability with the largest CPtr. This allows for any potential free slots
 * at the end of cspace to be found */
seL4_CPtr simple_last_valid_cap(simple_t *simple);

void simple_make_vka(simple_t *simple, vka_t *vka);

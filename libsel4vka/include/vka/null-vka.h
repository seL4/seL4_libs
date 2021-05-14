/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <vka/vka.h>

/* Initialise an allocator that always fails. This can be useful when you just
 * need to pacify something that expects VKA.
 */
void vka_init_nullvka(vka_t *vka);


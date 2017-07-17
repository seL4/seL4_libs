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

#include <vka/vka.h>
#include <vspace/vspace.h>
#include <platsupport/io.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * Creates a new implementation of the platsupport io_mapper interface using a
 * provided simple, vspace and vka
 *
 * @param vspace VSpace interface to use for mapping
 * @param vka VKA interface for allocating physical frames, and any extra objects or cslots
 * @param io_mapper Interface to fill in
 *
 * @return returns 0 on success
 */
int sel4platsupport_new_io_mapper(vspace_t vspace, vka_t vka, ps_io_mapper_t *io_mapper);

/**
 * Create a new malloc ops using stdlib malloc, calloc and free.
 *
 * @param ops Interface to fill in
 * @return    returns 0 on success
 */
int sel4platsupport_new_malloc_ops(ps_malloc_ops_t *ops);

/**
 * Creates a new implementation of the platsupport io_ops interface using a
 * provided vspace and vka
 *
 * @param vspace VSpace interface to use for mapping
 * @param vka VKA interface for allocating physical frames, and any extra objects or cslots
 * @param io_ops Interface to fill in
 *
 * @return returns 0 on success
 */
int sel4platsupport_new_io_ops(vspace_t vspace, vka_t vka, ps_io_ops_t *io_ops);

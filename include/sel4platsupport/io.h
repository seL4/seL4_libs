/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _SEL4_PLATSUPPORT_IO_H
#define _SEL4_PLATSUPPORT_IO_H

#include <vka/vka.h>
#include <vspace/vspace.h>
#include <simple/simple.h>
#include <platsupport/io.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * Creates a new implementation of the platsupport io_mapper interface using a
 * provided simple, vspace and vka
 *
 * @param simple Simple interface for getting the physical frames
 * @param vspace VSpace interface to use for mapping
 * @param vka VKA interface for allocating any extra objects or cslots
 * @param io_mapper Interface to fill in
 *
 * @return returns 0 on success
 */
int sel4platsupport_new_io_mapper(simple_t simple, vspace_t vspace, vka_t vka, ps_io_mapper_t *io_mapper);

#endif /* _SEL4_PLATSUPPORT_IO_H */


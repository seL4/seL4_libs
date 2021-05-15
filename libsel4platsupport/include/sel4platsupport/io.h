/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <vka/vka.h>
#include <vspace/vspace.h>
#include <platsupport/io.h>
#include <simple/simple.h>
#include <sel4platsupport/irq.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>

/*
 * NOTE: All vspace, vka, simple interfaces that are passed in *MUST* remain
 * valid for the lifetime of these interfaces.
 *
 * Additionally, these interfaces are not thread-safe, so care must be taken to
 * ensure that synchronisation is controlled externally. This also includes the
 * simple, vspace and vka interfaces that were passed in to initialise these
 * interfaces.
 */

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
int sel4platsupport_new_io_mapper(vspace_t *vspace, vka_t *vka, ps_io_mapper_t *io_mapper);

/**
 * Create a new malloc ops using stdlib malloc, calloc and free.
 *
 * @param ops Interface to fill in
 * @return    returns 0 on success
 */
int sel4platsupport_new_malloc_ops(ps_malloc_ops_t *ops);

/**
 * Create a new FDT ops structure using a provided simple
 *
 * @param io_fdt Interface to fill in
 * @param simple An initialised simple interface
 *
 * @return returns 0 on success
 */
int sel4platsupport_new_fdt_ops(ps_io_fdt_t *io_fdt, simple_t *simple, ps_malloc_ops_t *malloc_ops);

/**
 * Creates a new implementation of the platsupport io_ops interface using a
 * provided vspace and vka. Keep in mind that the more device specific
 * interfaces like MUX and CLK subsystems are not initialised.
 *
 * @param vspace VSpace interface to use for mapping
 * @param vka VKA interface for allocating physical frames, and any extra objects or cslots
 * @param simple A simple interface to access init caps from
 * @param io_ops Interface to fill in
 *
 * @return returns 0 on success
 */
int sel4platsupport_new_io_ops(vspace_t *vspace, vka_t *vka, simple_t *simple, ps_io_ops_t *io_ops);

/* Initialise all arch-specific io ops for this platform
 *
 * sel4platsupport_new_io_ops should have already populated relevant non-arch specific
 * io ops (memory allocation, mapping).
 *
 * @param simple a simple interface
 * @param vka a vka interface
 * @return 0 on success.
 */
int sel4platsupport_new_arch_ops(ps_io_ops_t *ops, simple_t *simple, vka_t *vka);

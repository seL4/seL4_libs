/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <sel4/sel4.h>
#include <platsupport/io.h>
#include <simple/simple.h>
#include <vka/vka.h>

/*
 * Get an io_port_ops interface.
 *
 * @param ops pointer to the interface struct to fill in
 * @param simple Simple interface that will be used to retrieve IOPort capabilities. This pointer
 *               needs to remain valid for as long as you use the filled in interface
 * @param vka   VKA interface for allocating capabilities and other reswources. This pointer needs
 *              to remain valid for as long as you used the filled in interface
 *
 * @return 0 on success.
 */
int sel4platsupport_get_io_port_ops(ps_io_port_ops_t *ops, simple_t *simple, vka_t *vka);


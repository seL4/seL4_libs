/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sel4/sel4.h>
#include <platsupport/io.h>
#include <sel4platsupport/io.h>
#include <sel4platsupport/arch/io.h>
#include <utils/util.h>

int sel4platsupport_new_arch_ops(UNUSED ps_io_ops_t *ops, UNUSED simple_t *simple, UNUSED vka_t *vka)
{
    return 0;
}


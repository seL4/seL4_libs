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

#include <sel4/sel4.h>
#include <platsupport/io.h>
#include <sel4platsupport/io.h>
#include <sel4platsupport/arch/io.h>
#include <utils/util.h>

int sel4platsupport_new_arch_ops(UNUSED ps_io_ops_t *ops, UNUSED simple_t *simple)
{
    return 0;
}


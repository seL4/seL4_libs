/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <vspace/vspace.h>

/**
 * Allocate a new stack and start running func on it.
 * If func returns, you will be back on the old stack.
 *
 * @param vspace interface to allocate stack with
 * @param func to jump to with the new stack.
 * @param arg to pass to func.
 * @param[out] retval The return value of func is written to this pointer if it is non-null
 * @ret   0 on success or -1 if stack allocation fails.
 *
 */
int sel4utils_run_on_stack(vspace_t *vspace, void * (*func)(void *arg), void *arg, void **retval);


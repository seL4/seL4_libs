/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef __SEL4UTILS_STACK_H
#define __SEL4UTILS_STACK_H

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

#endif /* __SEL4UTILS_STACK_H */

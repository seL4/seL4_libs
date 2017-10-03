/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#pragma once

#include "vmm.h"

typedef int (*vmcall_handler)(vmm_vcpu_t *vcpu);

/*
    A handler for a given vmcall function
     Called in a vmcall exception if its token matches the vcpu register eax
*/
typedef struct vmcall_handler {
    int token;
    vmcall_handler func;
} vmcall_handler_t;

/*
    Simple functions for registering handlers,
        calling a handler
*/
int reg_new_handler(vmm_t *vmm, vmcall_handler func, int token);
int vmm_vmcall_handler(vmm_vcpu_t *vcpu);

/* Handlers that can be registered */
int vchan_handler(vmm_vcpu_t *vcpu);


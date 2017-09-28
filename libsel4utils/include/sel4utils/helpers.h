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

#include <sel4/types.h>
#include <sel4utils/thread.h>
#include <stdbool.h>
#include <vka/vka.h>
#include <vspace/vspace.h>

#if CONFIG_WORD_SIZE == 64
#define Elf_auxv_t Elf64_auxv_t
#elif CONFIG_WORD_SIZE == 32
#define Elf_auxv_t Elf32_auxv_t
#else
#error "Word size unsupported"
#endif /* CONFIG_WORD_SIZE */

/* write to a remote stack */
int sel4utils_stack_write(vspace_t *current_vspace, vspace_t *target_vspace,
                      vka_t *vka, void *buf, size_t len, uintptr_t *stack_top);

/*
 * Initialize a threads user context for a specific architecture
 *
 * Unlike sel4utils_arch_init_context_with_args, the specified entry_point
 * does not need to be a function.
 *
 * @return 0 on success.
 */
int sel4utils_arch_init_context(void *entry_point, void *stack_top, seL4_UserContext *context);

/*
 * Legacy function to initialise a threads user context for a specific architecture, and put
 * some arguments into registers/stack.
 *
 * stack_top must be aligned to STACK_CALL_ALIGNMENT
 *
 * On x86, entry_point must be the address of a function without the NORETURN attribute.
 * Specifically, the function must be compiled under the assumption that the return
 * address was pushed onto the stack when the function is called. We aren't going to call
 * the function, but we will align the stack pointer as if it was called. This is
 * important, since the compiler emits instructions that assume alignment of the stack
 * pointer, under the assumption that functions will be called (as opposed to jumped to).
 *
 * On arm, the restriction is relaxed, as the return address is not pushed onto the stack
 * when a function is called.
 *
 * @param local_stack true of the stack is mapped in the current address space. If local stack is
 *        false and we are running on x86 (32-bit) this function will not copy arg* unless vka,
 *        local_vspace and remote_vspace are provided.
 *
 * @return 0 on success.
 */
int sel4utils_arch_init_context_with_args(sel4utils_thread_entry_fn entry_point,
                                          void *arg0, void *arg1, void *arg2,
                                          bool local_stack, void *stack_top, seL4_UserContext *context,
                                          vka_t *vka, vspace_t *local_vspace, vspace_t *remote_vspace);

/* convenient wrappers */
static inline int
sel4utils_arch_init_local_context(sel4utils_thread_entry_fn entry_point,
                                  void *arg0, void *arg1, void *arg2,
                                  void *stack_top, seL4_UserContext *context)
{
    return sel4utils_arch_init_context_with_args(entry_point, arg0, arg1, arg2, true, stack_top, context, NULL, NULL, NULL);
}


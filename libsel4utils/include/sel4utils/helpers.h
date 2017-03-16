/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef SEL4UTILS_HELPERS_H
#define SEL4UTILS_HELPERS_H

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
 * @return 0 on success.
 */
int sel4utils_arch_init_context(void *entry_point, void *stack_top, seL4_UserContext *context);

/*
 * Legacy function to initialise a threads user context for a specific architecture, and put
 * some arguments into registers/stack
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

#endif /* SEL4UTILS_HELPERS_H */

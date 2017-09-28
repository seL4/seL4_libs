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

#include <sel4/sel4.h>

static inline int seL4_ARCH_PageDirectory_Clean_Data(seL4_CPtr root, seL4_Word start, seL4_Word end)
{
    return seL4_ARM_PageDirectory_Clean_Data(root, start, end);
}

static inline int seL4_ARCH_PageDirectory_Invalidate_Data(seL4_CPtr root, seL4_Word start, seL4_Word end)
{
    return seL4_ARM_PageDirectory_Invalidate_Data(root, start, end);
}

static inline int seL4_ARCH_PageDirectory_CleanInvalidate_Data(seL4_CPtr root, seL4_Word start, seL4_Word end)
{
    return seL4_ARM_PageDirectory_CleanInvalidate_Data(root, start, end);
}

static inline int seL4_ARCH_PageDirectory_Unify_Instruction(seL4_CPtr root, seL4_Word start, seL4_Word end)
{
    return seL4_ARM_PageDirectory_Unify_Instruction(root, start, end);
}


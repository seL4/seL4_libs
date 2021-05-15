/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <sel4/sel4.h>

static inline int seL4_ARCH_PageDirectory_Clean_Data(seL4_CPtr root, seL4_Word start, seL4_Word end)
{
    return seL4_ARM_VSpace_Clean_Data(root, start, end);
}

static inline int seL4_ARCH_PageDirectory_Invalidate_Data(seL4_CPtr root, seL4_Word start, seL4_Word end)
{
    return seL4_ARM_VSpace_Invalidate_Data(root, start, end);
}

static inline int seL4_ARCH_PageDirectory_CleanInvalidate_Data(seL4_CPtr root, seL4_Word start, seL4_Word end)
{
    return seL4_ARM_VSpace_CleanInvalidate_Data(root, start, end);
}

static inline int seL4_ARCH_PageDirectory_Unify_Instruction(seL4_CPtr root, seL4_Word start, seL4_Word end)
{
    return seL4_ARM_VSpace_Unify_Instruction(root, start, end);
}


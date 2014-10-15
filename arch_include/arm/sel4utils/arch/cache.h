/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef SEL4UTILS_ARCH_CACHE_H
#define SEL4UTILS_ARCH_CACHE_H

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

#endif /* SEL4UTILS_ARCH_CACHE_H */



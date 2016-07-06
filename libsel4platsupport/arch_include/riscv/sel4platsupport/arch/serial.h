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

#include <vka/object.h>

typedef struct arch_serial_objects {
    /* Paddr of the platsupport default serial's frame */
    uintptr_t serial_frame_paddr;
    /* VKA object for the platsupport default serial's frame */
    vka_object_t serial_frame_obj;
} arch_serial_objects_t;

/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <vka/vka.h>
#include <allocman/allocman.h>

/**
 * Make a VKA object using this allocman
 *
 * @param vka structure for the vka interface object
 * @param alloc allocator to be used with this vka
 */

void allocman_make_vka(vka_t *vka, allocman_t *alloc);

/**
 * Make an allocman from a VKA
 * This constructs an allocman that has a cspace and utspace
 * that forward to the VKA interface, and fullfills the mspace
 * with the malloc mspace implementation
 * The pointer to the VKA is kept and needs to remain valid
 *
 * @param vka Interface to proxy calls to
 * @param alloc Interface to fill out
 * @return 0 on success
 */
int allocman_make_from_vka(vka_t *vka, allocman_t *alloc);


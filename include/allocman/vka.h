/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __AM_VKA__
#define __AM_VKA__

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

#endif

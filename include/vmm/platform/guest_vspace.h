/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef __LIB_VMM_PLATFORM_GUEST_VSPACE_H__
#define __LIB_VMM_PLATFORM_GUEST_VSPACE_H__

#include <autoconf.h>
#include <sel4/sel4.h>
#include <vspace/vspace.h>
#include <vka/vka.h>

/* Constructs a vspace that will duplicate mappings between an ept
 * and several IO spaces */
int vmm_get_guest_vspace(vspace_t *loader, vspace_t *new_vspace, vka_t *vka, seL4_CPtr page_directory);

#ifdef CONFIG_IOMMU
/* Attach an additional IO space to the vspace */
int vmm_guest_vspace_add_iospace(vspace_t *vspace, seL4_CPtr iospace);
#endif

#endif /* __LIB_VMM_PLATFORM_GUEST_VSPACE_H__ */

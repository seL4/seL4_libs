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

#include <platsupport/io.h>
#include <ethdrivers/raw.h>
#include <vmm/platform/guest_vspace.h>

struct ethif_virtio_emul_internal;

typedef struct ethif_virtio_emul {
    /* pointer to internal information */
    struct ethif_virtio_emul_internal *internal;
    /* io port interface functions */
    int (*io_in)(struct ethif_virtio_emul *emul, unsigned int offset, unsigned int size, unsigned int *result);
    int (*io_out)(struct ethif_virtio_emul *emul, unsigned int offset, unsigned int size, unsigned int value);
    /* notify of a status change in the underlying driver.
     * typically this would be due to link coming up
     * meaning that transmits can finally happen */
    int (*notify)(struct ethif_virtio_emul *emul);
} ethif_virtio_emul_t;

ethif_virtio_emul_t *ethif_virtio_emul_init(ps_io_ops_t io_ops, int queue_size, vspace_t *guest_vspace, ethif_driver_init driver, void *config);


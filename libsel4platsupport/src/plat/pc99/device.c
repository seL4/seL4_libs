/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#include <autoconf.h>
#include <sel4platsupport/device.h>
#include <sel4platsupport/platsupport.h>
#include <vka/capops.h>
#include <utils/util.h>

seL4_Error
sel4platsupport_copy_msi_cap(vka_t *vka, simple_t *simple, seL4_Word irq_number, cspacepath_t *dest)
{
    seL4_CPtr irq;

    /* allocate a cslot for the irq cap */
    int error = vka_cspace_alloc(vka, &irq);
    if (error != 0) {
        ZF_LOGE("Failed to allocate cslot for msi\n");
        return error;
    }

    vka_cspace_make_path(vka, irq, dest);

    error = arch_simple_get_msi(&simple->arch_simple, *dest, 0, 0, 0, 0, irq_number);
    if  (error != seL4_NoError) {
        ZF_LOGE("Failed to get cap to msi irq_number %zu\n", irq_number);
        vka_cspace_free(vka, irq);
        return error;
    }

    return seL4_NoError;
}
 

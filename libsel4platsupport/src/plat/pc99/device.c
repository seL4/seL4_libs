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

seL4_Error
sel4platsupport_copy_ioapic_cap(vka_t *vka, simple_t *simple, seL4_Word ioapic, seL4_Word pin, seL4_Word level,
                                seL4_Word polarity, seL4_Word vector, cspacepath_t *dest)
{
    seL4_CPtr irq;

    /* allocate a cslot for the irq cap */
    int error = vka_cspace_alloc(vka, &irq);
    if (error != 0) {
        ZF_LOGE("Failed to allocate cslot for msi");
        return error;
    }

    vka_cspace_make_path(vka, irq, dest);

    error = arch_simple_get_ioapic(&simple->arch_simple, *dest, ioapic, pin, level, polarity, vector);
    if  (error != seL4_NoError) {
        ZF_LOGE("Failed to get requested irq cap");
        vka_cspace_free(vka, irq);
        return error;
    }

    return seL4_NoError;
}

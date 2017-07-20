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
#include <platsupport/irq.h>
#include <sel4platsupport/serial.h>
#include <sel4platsupport/device.h>

int
sel4platsupport_arch_init_default_serial_caps(vka_t *vka, UNUSED vspace_t *vspace, simple_t *simple, serial_objects_t *serial_objects)
{
    int error;

    ps_irq_t irq = {
        .type = PS_INTERRUPT,
        .irq.number = DEFAULT_SERIAL_INTERRUPT
    };

    /* Obtain IRQ cap for PS default serial. */
    error = sel4platsupport_copy_irq_cap(vka, simple, &irq,
                                           &serial_objects->serial_irq_path);
    if (error) {
        ZF_LOGF("Failed to obtain PS default serial IRQ cap.");
        return error;
    }

    /* Obtain frame cap for PS default serial.
     * We pass the serial's MMIO frame as a frame object, and not an untyped,
     * like the way we pass the timer MMIO paddr. The reason for this is that
     * the child tests use the serial device themselves, and we can't retype
     * an untyped twice. But we can make copies of a Frame cap.
     */
    serial_objects->arch_serial_objects.serial_frame_paddr = DEFAULT_SERIAL_PADDR;
    error = vka_alloc_frame_at(vka, seL4_PageBits, DEFAULT_SERIAL_PADDR,
                               &serial_objects->arch_serial_objects.serial_frame_obj);
    if (error) {
        ZF_LOGF("Failed to obtain frame cap for default serial.");
        return error;
    }

    return 0;
}

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

#include <sel4platsupport/serial.h>

int
sel4platsupport_arch_init_default_serial_caps(vka_t *vka, UNUSED vspace_t *vspace, simple_t *simple, serial_objects_t *serial_objects)
{
    int error;

    /* Fill in the IO port caps for each of the serial devices.
     *
     * There's no need to allocate cspace slots because all of these just
     * actually boil down into calls to simple_default_get_IOPort_cap(), which
     * simply returns seL4_CapIOPort.
     *
     * But if in the future, somebody refines libsel4simple-default to actually
     * retype and sub-partition caps, we want to be doing the correct thing.
     */
    error = vka_cspace_alloc(vka, &serial_objects->arch_serial_objects.serial_io_port_cap);
    if (error != 0) {
        ZF_LOGE("Failed to alloc slot for COM1 port cap.");
        return error;
    }

    /* Also allocate the serial IRQ. This is placed in the arch-specific code
     * because x86 also allows the EGA device to be defined as a
     * PS_SERIAL_DEFAULT device, so since it does not have an IRQ, we have to
     * do arch-specific processing for that case.
     */
    if (PS_SERIAL_DEFAULT != PC99_TEXT_EGA) {
        /* The slot was allocated earlier, outside in init_serial_caps. */
        error = simple_get_IRQ_handler(simple,
                                       DEFAULT_SERIAL_INTERRUPT,
                                       serial_objects->serial_irq_path);
        if (error != 0) {
            ZF_LOGE("Failed to get IRQ cap for default COM device. IRQ is %d.",
                    DEFAULT_SERIAL_INTERRUPT);
            return error;
        }
    } else {
        cspacepath_t tmp = { 0 };
        serial_objects->serial_irq_path = tmp;
    }

    serial_objects->arch_serial_objects.serial_io_port_cap = simple_get_IOPort_cap(simple,
                                                     SERIAL_CONSOLE_COM1_PORT,
                                                     SERIAL_CONSOLE_COM1_PORT_END);
    if (serial_objects->arch_serial_objects.serial_io_port_cap == 0) {
        ZF_LOGE("Failed to get COM1 port cap.");
        if(PS_SERIAL_DEFAULT == PS_SERIAL0) {
            ZF_LOGW("COM1 is the default serial.");
        }
        return -1;
    }

    return 0;
}

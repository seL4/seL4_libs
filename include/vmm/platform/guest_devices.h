/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/* The Guest OS device configuration.
 *
 *     Authors:
 *         Qian Ge
 *         Adrian Danis
 *         Xi (Ma) Chen
 *
 *     Tue 05 Nov 2013 14:42:17 EST 
 */
 
#ifndef __LIB_VMM_GUEST_DEVICE_CONFIGURATION_H_
#define __LIB_VMM_GUEST_DEVICE_CONFIGURATION_H_

#include <stdint.h>
#include <stdbool.h>
#include <sel4/sel4.h>

#include "vmm/config.h"
#include "vmm/io.h"

#if defined(VMM_HARDWARE_VMWARE) || defined(CONFIG_VMM_HARDWARE_LITTLEBIRD)

#define VMM_PCI_PASSTHROUGH_ALL_PCI_DEVICES false
#define VMM_PCI_DEFAULT_MAP_OFFSET -0x30000000UL

typedef struct vmm_guest_device_cfg_ {
    uint16_t ven;
    uint16_t dev;
    bool mem_map;
    bool io_map;
    int64_t map_offset;
    bool pci_cfg_extended;
} vmm_guest_device_cfg_t;

static vmm_guest_device_cfg_t vmm_plat_guest_allowed_devices[]  _UNUSED_ =
{

    /* Allow Intel hostbridge devices to keep Linux happy. */
    {.ven = 0x8086, .dev = 0x7190, .mem_map = false},
    {.ven = 0x8086, .dev = 0x7191, .mem_map = false},
    {.ven = 0x8086, .dev = 0x7110, .mem_map = false},
    {.ven = 0x8086, .dev = 0x7111, .mem_map = false},
    {.ven = 0x8086, .dev = 0x7113, .mem_map = false},

    // Allow the VMware PCI bridge.
    {.ven = 0x15ad, .dev = 0x0790, .mem_map = false},

    // Allow the NCR LSI53c1030 SCSI device.
    {.ven = 0x1000, .dev = 0x0030, .mem_map = true, .io_map = true,
     .map_offset = VMM_PCI_DEFAULT_MAP_OFFSET, .pci_cfg_extended = true},

    // Allow the AMD Lance Ethernet controller device.
    {.ven = 0x1022, .dev = 0x2000, .mem_map = true, .io_map = true},
    
#ifndef LIB_VMM_VESA_FRAMEBUFFER
    /* Allow the VMWare SVGA video device. */
    {.ven = 0x15ad, .dev = 0x0405, .mem_map = true, .io_map = true,
     .map_offset = VMM_PCI_DEFAULT_MAP_OFFSET, .pci_cfg_extended = true},
#endif

#ifndef LIB_VMM_VESA_FRAMEBUFFER
    /* Intel Integrated Graphics */
//    {.ven = 0x8086, .dev = 0x0044,
//      .mem_map = true, .io_map = true, .map_offset = VMM_PCI_DEFAULT_MAP_OFFSET, .pci_cfg_extended = true},
//    {.ven = 0x8086, .dev = 0x0046,
//      .mem_map = true, .io_map = true, .map_offset = VMM_PCI_DEFAULT_MAP_OFFSET, .pci_cfg_extended = true},
#endif

    /* Intel USB */
    {.ven = 0x8086, .dev = 0x3b34,
      .mem_map = true, .io_map = true, .map_offset = VMM_PCI_DEFAULT_MAP_OFFSET, .pci_cfg_extended = true},
    {.ven = 0x8086, .dev = 0x3b3c,
      .mem_map = true, .io_map = true, .map_offset = VMM_PCI_DEFAULT_MAP_OFFSET, .pci_cfg_extended = true},

    /* Intel HDA Audio */
    {.ven = 0x8086, .dev = 0x3b56,
      .mem_map = true, .io_map = true, .map_offset = VMM_PCI_DEFAULT_MAP_OFFSET, .pci_cfg_extended = true},

    /* SMBus (I2C) */
    {.ven = 0x8086, .dev = 0x3b30,
      .mem_map = true, .io_map = true, .map_offset = VMM_PCI_DEFAULT_MAP_OFFSET, .pci_cfg_extended = true},

    /* Random littlebird things */
//    {.ven = 0x8086, .dev = 0x3b42,
//      .mem_map = true, .io_map = true, .map_offset = VMM_PCI_DEFAULT_MAP_OFFSET, .pci_cfg_extended = true},
//    {.ven = 0x8086, .dev = 0x3b4a,
//      .mem_map = true, .io_map = true, .map_offset = VMM_PCI_DEFAULT_MAP_OFFSET, .pci_cfg_extended = true},
//    {.ven = 0x8086, .dev = 0x3b4e,
//      .mem_map = true, .io_map = true, .map_offset = VMM_PCI_DEFAULT_MAP_OFFSET, .pci_cfg_extended = true},
    {.ven = 0x8086, .dev = 0x3b32,
      .mem_map = true, .io_map = true, .map_offset = VMM_PCI_DEFAULT_MAP_OFFSET, .pci_cfg_extended = true},
//    {.ven = 0x12d8, .dev = 0xe130,
//      .mem_map = true, .io_map = true, .map_offset = VMM_PCI_DEFAULT_MAP_OFFSET, .pci_cfg_extended = true},

    /* VME Bus */
    {.ven = 0x10e3, .dev = 0x0148,
      .mem_map = true, .io_map = true, .map_offset = VMM_PCI_DEFAULT_MAP_OFFSET, .pci_cfg_extended = true},

    /* Intel SATA controller */
    {.ven = 0x8086, .dev = 0x3b2f,
      .mem_map = true, .io_map = true, .map_offset = VMM_PCI_DEFAULT_MAP_OFFSET, .pci_cfg_extended = true},

    /* Intel gigabit network */
    {.ven = 0x8086, .dev = 0x150e,
      .mem_map = true, .io_map = true, .map_offset = VMM_PCI_DEFAULT_MAP_OFFSET, .pci_cfg_extended = true},

    /* Mark end-of-list with 0xFFFF vendor. */
    {.ven = 0xFFFF, .dev = 0xFFFF, .mem_map = false}
};

#define LIB_VMM_CAP_IGNORE LIB_VMM_CAP_INVALID
#define LIB_VMM_IGNORE_UNDEFINED_IOPORTS

/* Guest OS IO port to driver endpoint mapping. */
static ioport_range_t ioport_global_map[]  _UNUSED_ = {
    
    {X86_IO_SERIAL_1_START,   X86_IO_SERIAL_1_END,   LIB_VMM_CAP_INVALID,          X86_IO_PASSTHROUGH, "COM1 Serial Port"},
    {X86_IO_SERIAL_2_START,   X86_IO_SERIAL_2_END,   LIB_VMM_CAP_IGNORE,           X86_IO_PASSTHROUGH,       "COM2 Serial Port"},
    {X86_IO_SERIAL_3_START,   X86_IO_SERIAL_3_END,   LIB_VMM_CAP_IGNORE,           X86_IO_PASSTHROUGH,       "COM3 Serial Port"},
    {X86_IO_SERIAL_4_START,   X86_IO_SERIAL_4_END,   LIB_VMM_CAP_IGNORE,           X86_IO_PASSTHROUGH,       "COM4 Serial Port"},
    {X86_IO_PIC_1_START,      X86_IO_PIC_1_END,      LIB_VMM_DRIVER_INTERRUPT_CAP, X86_IO_CATCH,       "8259 Programmable Interrupt Controller (1st, Master)"},
    {X86_IO_PIC_2_START,      X86_IO_PIC_2_END,      LIB_VMM_DRIVER_INTERRUPT_CAP, X86_IO_CATCH,       "8259 Programmable Interrupt Controller (2nd, Slave)"},
    {X86_IO_ELCR_START,       X86_IO_ELCR_END,       LIB_VMM_DRIVER_INTERRUPT_CAP, X86_IO_CATCH,       "ELCR (edge/level control register) for IRQ line"},
    {X86_IO_PCI_CONFIG_START, X86_IO_PCI_CONFIG_END, LIB_VMM_DRIVER_PCI_CAP,       X86_IO_CATCH,       "PCI Configuration"},
    {X86_IO_RTC_START,        X86_IO_RTC_END,        LIB_VMM_CAP_INVALID,          X86_IO_PASSTHROUGH, "CMOS Registers / RTC Real-Time Clock / NMI Interrupts"},
    {X86_IO_PIT_START,        X86_IO_PIT_END,        LIB_VMM_CAP_INVALID,          X86_IO_PASSTHROUGH, "8253/8254 Programmable Interval Timer"},
    {X86_IO_PS2C_START,       X86_IO_PS2C_END,       LIB_VMM_CAP_INVALID,          X86_IO_PASSTHROUGH, "8042 PS/2 Controller"},
    {X86_IO_POS_START,        X86_IO_POS_END,        LIB_VMM_CAP_INVALID,          X86_IO_PASSTHROUGH, "POS Programmable Option Select (PS/2)"},

    {0xC000,                  0xF000,                LIB_VMM_CAP_IGNORE,           X86_IO_CATCH,       "PCI Bus IOPort Mapping Space"},
    {0x1060,                  0x1070,                LIB_VMM_CAP_IGNORE,           X86_IO_CATCH, "IDE controller"},
    {0x01F0,                  0x01F8,                LIB_VMM_CAP_IGNORE,           X86_IO_CATCH, "Primary IDE controller"},
    {0x0170,                  0x0178,                LIB_VMM_CAP_IGNORE,           X86_IO_CATCH, "Secondary IDE controller"},
    {0x3f6,                   0x03f7,                LIB_VMM_CAP_IGNORE,           X86_IO_CATCH, "Additional ATA register"},
    {0x376,                   0x0377,                LIB_VMM_CAP_IGNORE,           X86_IO_CATCH, "Additional ATA register"},
    {0x3b0,                   0x3df,                 LIB_VMM_CAP_IGNORE,           X86_IO_PASSTHROUGH, "IBM VGA"},

    {0x80,                    0x80,                  LIB_VMM_CAP_IGNORE,           X86_IO_CATCH, "DMA IOPort timer"},

    /* Digital PLD modules on Littlebird */
    {0x162e,                  0x162f,                LIB_VMM_CAP_IGNORE,           X86_IO_PASSTHROUGH, "PLD Registers"},
    {0x378,                   0x37f,                 LIB_VMM_CAP_IGNORE,           X86_IO_PASSTHROUGH, "PLD Discrete I/O"},
    {0x3f0,                   0x3f7,                 LIB_VMM_CAP_IGNORE,           X86_IO_PASSTHROUGH, "PLD Extended discrete I/O"},
    {0x164e,                  0x164f,                LIB_VMM_CAP_IGNORE,           X86_IO_PASSTHROUGH, "Serial Configuration Registers"},
    {0x160E,                  0x160F,                LIB_VMM_CAP_IGNORE,           X86_IO_PASSTHROUGH, "CANbus 1"},
    {0x161E,                  0x161F,                LIB_VMM_CAP_IGNORE,           X86_IO_PASSTHROUGH, "CANbus 2"},

    /* Mark end of list with IO_INVALID. */
    {X86_IO_INVALID,          X86_IO_INVALID,        LIB_VMM_CAP_INVALID,          X86_IO_CATCH,       "Invalid"}
};

#else /* VMM_HARDWARE_VMWARE */
    #error "Undefined hardware platform."
#endif /* VMM_HARDWARE_VMWARE */

#endif /* __LIB_VMM_GUEST_DEVICE_CONFIGURATION_H_ */

/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#define ACPI_START (0xE0000) // Start of ACPI tables; RSD PTR is right here
#define MAX_ACPI_TABLES (2)

int make_guest_acpi_tables(vmm_t *vmm);


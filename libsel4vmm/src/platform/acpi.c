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

/* Routines for generating guest ACPI tables.
Author: W.A. */

#include <stdlib.h>
#include <string.h>

#include "vmm/debug.h"
#include "vmm/platform/acpi.h"
#include "vmm/platform/guest_memory.h"
#include "vmm/platform/guest_vspace.h"
#include "vmm/processor/apicdef.h"

#include "platsupport/plat/acpi/acpi.h"

#define APIC_FLAGS_ENABLED (1)

uint8_t acpi_calc_checksum(const char* table, int length)
{
    uint32_t sum = 0;

    for (int i = 0; i < length; i++) {
        sum += *table++;
    }

    return 0x100 - (sum & 0xFF);
}

static void acpi_fill_table_head(acpi_header_t *head, const char *signature, uint8_t rev) {
    const char *oem = "NICTA ";
    const char *padd = "    ";
    memcpy(head->signature, signature, sizeof(head->signature));
    memcpy(head->oem_id, oem, sizeof(head->oem_id));
    memcpy(head->creator_id, oem, sizeof(head->creator_id));
    memcpy(head->oem_table_id, signature, sizeof(head->signature));
    memcpy(head->oem_table_id + sizeof(head->signature), padd, 4);

    head->revision = rev;
    head->checksum = 0;
    head->length = sizeof(*head);
    head->oem_revision = rev;
    head->creator_revision = 1;
}

static int make_guest_acpi_tables_continued(uintptr_t paddr, void *vaddr,
        size_t size, size_t offset, void *cookie) {
    (void)offset;
    memcpy((char *)vaddr, (char *)cookie, size);
    return 0;
}

// Give some ACPI tables to the guest
int make_guest_acpi_tables(vmm_t *vmm) {
    DPRINTF(2, "Making ACPI tables\n");

    int cpus = vmm->num_vcpus;

    int err;

    // XSDT and other tables
    void *tables[MAX_ACPI_TABLES];
    size_t table_sizes[MAX_ACPI_TABLES];
    int num_tables = 1;

    // MADT
    int madt_size = sizeof(acpi_madt_t)
        /* + sizeof(acpi_madt_ioapic_t)*/
        + sizeof(acpi_madt_local_apic_t) * cpus;
    acpi_madt_t *madt = malloc(madt_size);
    acpi_fill_table_head(&madt->header, "APIC", 3);
    madt->local_int_crt_address = APIC_DEFAULT_PHYS_BASE;
    madt->flags = 1;

    char *madt_entry = (char *)madt + sizeof(acpi_madt_t);

#if 0
    acpi_madt_ioapic_t ioapic = { // MADT IOAPIC entry
        .header = {
            .type = ACPI_APIC_IOAPIC,
            .length = sizeof(acpi_madt_ioapic_t)
        },
        .ioapic_id = 0,
        .address = IOAPIC_DEFAULT_PHYS_BASE,
        .gs_interrupt_base = 0 // TODO set this up?
    };
    memcpy(madt_entry, &ioapic, sizeof(ioapic));
    madt_entry += sizeof(ioapic);
#endif

    for (int i = 0; i < cpus; i++) { // MADT APIC entries
        acpi_madt_local_apic_t apic = {
            .header = {
                .type = ACPI_APIC_LOCAL,
                .length = sizeof(acpi_madt_local_apic_t)
            },
            .processor_id = i + 1,
            .apic_id = i,
            .flags = APIC_FLAGS_ENABLED
        };
        memcpy(madt_entry, &apic, sizeof(apic));
        madt_entry += sizeof(apic);
    }

    madt->header.length = madt_size;
    madt->header.checksum = acpi_calc_checksum((char *)madt, madt_size);

    tables[num_tables] = madt;
    table_sizes[num_tables] = madt_size;
    num_tables++;

    // Could set up other tables here...

    // XSDT
    size_t xsdt_size = sizeof(acpi_xsdt_t) + sizeof(uint64_t) * (num_tables - 1);

    // How much space will all the tables take up?
    size_t acpi_size = xsdt_size;
    for (int i = 1; i < num_tables; i++) {
        acpi_size += table_sizes[i];
    }

    // Allocate some frames for this
    uintptr_t xsdt_addr = 0xe1000; // TODO actually allocate frames, can be anywhere

    acpi_xsdt_t *xsdt = malloc(xsdt_size);
    acpi_fill_table_head(&xsdt->header, "XSDT", 1);

    // Add previous tables to XSDT pointer list
    uintptr_t table_paddr = xsdt_addr + xsdt_size;
    uint64_t *entry = (uint64_t *)((char *)xsdt + sizeof(acpi_xsdt_t));
    for (int i = 1; i < num_tables; i++) {
        *entry++ = (uint64_t)table_paddr;
        table_paddr += table_sizes[i];
    }

    xsdt->header.length = xsdt_size;
    xsdt->header.checksum = acpi_calc_checksum((char *)xsdt, xsdt_size);

    tables[0] = xsdt;
    table_sizes[0] = xsdt_size;

    // Copy all the tables to guest
    table_paddr = xsdt_addr;
    for (int i = 0; i < num_tables; i++) {
        DPRINTF(2, "ACPI table \"%.4s\", addr = %p, size = %zu bytes\n",
                (char *)tables[i], (void*)table_paddr, table_sizes[i]);
        err = vmm_guest_vspace_touch(&vmm->guest_mem.vspace, table_paddr,
                table_sizes[i], make_guest_acpi_tables_continued, tables[i]);
        if (err) {
            return err;
        }
        table_paddr += table_sizes[i];
    }

    // RSDP
    uintptr_t rsdp_addr = ACPI_START;
    err = vmm_alloc_guest_device_at(vmm, ACPI_START, sizeof(acpi_rsdp_t));
    if (err) {
        return err;
    }

    acpi_rsdp_t rsdp = {
        .signature = "RSD PTR ",
        .oem_id = "NICTA ",
        .revision = 2, /* ACPI v3*/
        .checksum = 0,
        .rsdt_address = xsdt_addr,
        /* rsdt_addrss will not be inspected as the xsdt is present.
           This is not ACPI 1 compliant */
        .length = sizeof(acpi_rsdp_t),
        .xsdt_address = xsdt_addr,
        .extended_checksum = 0,
        .reserved = {0}
    };

    rsdp.checksum = acpi_calc_checksum((char *)&rsdp, 20);
    rsdp.extended_checksum = acpi_calc_checksum((char *)&rsdp, sizeof(rsdp));

    DPRINTF(2, "ACPI RSDP addr = %p\n", (void*)rsdp_addr);

    return vmm_guest_vspace_touch(&vmm->guest_mem.vspace, rsdp_addr, sizeof(rsdp),
            make_guest_acpi_tables_continued, &rsdp);
}

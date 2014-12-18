#include <stdlib.h>
#include <string.h>

#include "vmm/debug.h"
#include "vmm/platform/acpi.h"
#include "vmm/platform/guest_memory.h"
#include "vmm/platform/guest_vspace.h"

static uint8_t acpi_checksum(void *table, int length) {
	uint32_t sum = 0;
	uint8_t *ctable = table;

	for (int i = 0; i < length; i++) {
		sum += *ctable++;
	}

	return 0x100 - (sum & 0xFF);
}

static void acpi_fill_table_head(acpi_table_head_t *head, const char *signature, uint8_t rev) {
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
	head->oem_rev = rev;
	head->creator_rev = 1;
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

	int cpus = 1; // TODO should come from vmm struct

	int err;

	// XSDT and other tables
	void *tables[MAX_ACPI_TABLES];
	size_t table_sizes[MAX_ACPI_TABLES];
	int num_tables = 1;

	// MADT
	int madt_size = sizeof(acpi_madt_t) + sizeof(acpi_madt_ioapic_t)
		+ sizeof(acpi_madt_apic_t) * cpus;
	acpi_madt_t *madt = malloc(madt_size);
	acpi_fill_table_head(&madt->head, "APIC", 3);
	madt->apic_addr = APIC_ADDR;
	madt->flags = 1;

	char *madt_entry = (char *)madt + sizeof(acpi_madt_t);

#if 0
	acpi_madt_ioapic_t ioapic = { // MADT IOAPIC entry
		.header = {
			.type = MADT_IOAPIC,
			.length = sizeof(acpi_madt_ioapic_t)
		},
		.ioapic_id = 0,
		.ioapic_addr = IOAPIC_ADDR,
		.gsib = 0 // TODO set this up?
	};
	memcpy(madt_entry, &ioapic, sizeof(ioapic));
	madt_entry += sizeof(ioapic);
#endif

	for (int i = 0; i < cpus; i++) { // MADT APIC entries
		acpi_madt_apic_t apic = {
			.header = {
				.type = MADT_APIC,
				.length = sizeof(acpi_madt_apic_t)
			},
			.cpu_id = i + 1,
			.apic_id = i,
			.flags = APIC_FLAGS_ENABLED
		};
		memcpy(madt_entry, &apic, sizeof(apic));
		madt_entry += sizeof(apic);
	}

	madt->head.length = madt_size;
	madt->head.checksum = acpi_checksum(madt, madt_size);

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
	uintptr_t xsdt_addr = 0xe1000; // TODO actually allocate frames

	acpi_xsdt_t *xsdt = malloc(xsdt_size);
	acpi_fill_table_head(&xsdt->head, "XSDT", 1);
	
	// Add previous tables to XSDT pointer list
	uintptr_t table_paddr = xsdt_addr + xsdt_size;
	uint64_t *entry = (uint64_t *)((char *)xsdt + sizeof(acpi_xsdt_t));
	for (int i = 1; i < num_tables; i++) {
		*entry++ = (uint64_t)table_paddr;
		table_paddr += table_sizes[i];
	}
	
	xsdt->head.length = xsdt_size;
	xsdt->head.checksum = acpi_checksum(xsdt, xsdt_size);
	
	tables[0] = xsdt;
	table_sizes[0] = xsdt_size;

	// Copy all the tables to guest
	table_paddr = xsdt_addr;
	for (int i = 0; i < num_tables; i++) {
		DPRINTF(2, "ACPI table \"%.4s\", addr = %08x, size = %d bytes\n",
				(char *)tables[i], table_paddr, table_sizes[i]);
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
	
	acpi_rsdx_t rsdp = {
		.rsdp = {
			.signature = "RSD PTR ",
			.oem_id = "NICTA ",
			.revision = 2, /* ACPI v3*/
			.checksum = 0,
			.rsdt_address = xsdt_addr
			/* rsdt_addrss will not be inspected as the xsdt is present.
			   This is not ACPI 1 compliant */
		},
		.length = sizeof(acpi_rsdp_t),
		.xsdt_address = xsdt_addr,
		.extended_checksum = 0,
		.reserved = {0}
	};

	rsdp.rsdp.checksum = acpi_checksum(&rsdp.rsdp, sizeof(rsdp.rsdp));	
	rsdp.extended_checksum = acpi_checksum(&rsdp, sizeof(rsdp));

	DPRINTF(2, "ACPI RSDP addr = %08x\n", rsdp_addr);

	return vmm_guest_vspace_touch(&vmm->guest_mem.vspace, rsdp_addr, sizeof(rsdp),
			make_guest_acpi_tables_continued, &rsdp);
}


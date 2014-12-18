// Based on from kernel/src/plat/pc99/machine/acpi.c

#define ACPI_START 0xE0000 // Start of ACPI tables; RSD PTR is right here
#define MAX_ACPI_TABLES 2

/* Root System Descriptor Pointer */
typedef struct acpi_rsdp {
    char         signature[8];
    uint8_t      checksum;
    char         oem_id[6];
    uint8_t      revision;
    uint32_t     rsdt_address;
} acpi_rsdp_t;

typedef struct acpi_rsdx {
	acpi_rsdp_t  rsdp;
    uint32_t     length;
    uint64_t     xsdt_address; // Only a 32 bit value
    uint8_t      extended_checksum;
    char         reserved[3];
} acpi_rsdx_t;

typedef struct acpi_table_head {
	char		signature[4];
	uint32_t	length;
	uint8_t		revision;
	uint8_t		checksum;
	char		oem_id[6];
	char		oem_table_id[8];
	char		oem_rev[4];
	char		creator_id[4];
	char		creator_rev[4];
} acpi_table_head_t;

typedef struct acpi_xsdt {
	acpi_table_head_t	head;
} acpi_xsdt_t;

/* Multiple APIC Description Table (MADT) */
typedef struct acpi_madt {
    acpi_table_head_t	head;
    uint32_t			apic_addr;
    uint32_t			flags;
} acpi_madt_t;

typedef struct acpi_madt_header {
    uint8_t type;
    uint8_t length;
} acpi_madt_header_t;

enum acpi_table_madt_struct_type {
    MADT_APIC   = 0,
    MADT_IOAPIC = 1,
    MADT_ISO    = 2,
};

typedef struct acpi_madt_apic {
    acpi_madt_header_t header;
    uint8_t            cpu_id;
    uint8_t            apic_id;
    uint32_t           flags;
} acpi_madt_apic_t;

typedef struct acpi_madt_ioapic {
    acpi_madt_header_t header;
    uint8_t            ioapic_id;
    uint8_t            reserved[1];
    uint32_t           ioapic_addr;
    uint32_t           gsib;
} acpi_madt_ioapic_t;

typedef struct acpi_madt_iso {
    acpi_madt_header_t header;
    uint8_t            bus; /* always 0 (ISA) */
    uint8_t            source;
    uint32_t           gsi;
    uint16_t           flags;
} acpi_madt_iso_t;


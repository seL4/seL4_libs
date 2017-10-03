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

/* Pretty sure most of this header was ripped straight from Linux */

#pragma once

#define EDD_MBR_SIG_MAX 16        /* max number of signatures to store */
#define E820MAX 128     /* number of entries in E820MAP */
#define EDDMAXNR 6      /* number of edd_info structs starting at EDDBUF  */

#define VIDEO_TYPE_MDA          0x10    /* Monochrome Text Display      */
#define VIDEO_TYPE_CGA          0x11    /* CGA Display                  */
#define VIDEO_TYPE_EGAM         0x20    /* EGA/VGA in Monochrome Mode   */
#define VIDEO_TYPE_EGAC         0x21    /* EGA in Color Mode            */
#define VIDEO_TYPE_VGAC         0x22    /* VGA+ in Color Mode           */
#define VIDEO_TYPE_VLFB         0x23    /* VESA VGA in graphic mode     */
#define VIDEO_TYPE_PICA_S3      0x30    /* ACER PICA-61 local S3 video  */
#define VIDEO_TYPE_MIPS_G364    0x31    /* MIPS Magnum 4000 G364 video  */
#define VIDEO_TYPE_SGI          0x33    /* Various SGI graphics hardware */
#define VIDEO_TYPE_TGAC         0x40    /* DEC TGA */
#define VIDEO_TYPE_SUN          0x50    /* Sun frame buffer. */
#define VIDEO_TYPE_SUNPCI       0x51    /* Sun PCI based frame buffer. */
#define VIDEO_TYPE_PMAC         0x60    /* PowerMacintosh frame buffer. */
#define VIDEO_TYPE_EFI          0x70    /* EFI graphic mode             */

struct screen_info {
    uint8_t  orig_x;       /* 0x00 */
    uint8_t  orig_y;       /* 0x01 */
    uint16_t ext_mem_k;    /* 0x02 */
    uint16_t orig_video_page;  /* 0x04 */
    uint8_t  orig_video_mode;  /* 0x06 */
    uint8_t  orig_video_cols;  /* 0x07 */
    uint8_t  flags;        /* 0x08 */
    uint8_t  unused2;      /* 0x09 */
    uint16_t orig_video_ega_bx;/* 0x0a */
    uint16_t unused3;      /* 0x0c */
    uint8_t  orig_video_lines; /* 0x0e */
    uint8_t  orig_video_isVGA; /* 0x0f */
    uint16_t orig_video_points;/* 0x10 */

    /* VESA graphic mode -- linear frame buffer */
    uint16_t lfb_width;    /* 0x12 */
    uint16_t lfb_height;   /* 0x14 */
    uint16_t lfb_depth;    /* 0x16 */
    uint32_t lfb_base;     /* 0x18 */
    uint32_t lfb_size;     /* 0x1c */
    uint16_t cl_magic, cl_offset; /* 0x20 */
    uint16_t lfb_linelength;   /* 0x24 */
    uint8_t  red_size;     /* 0x26 */
    uint8_t  red_pos;      /* 0x27 */
    uint8_t  green_size;   /* 0x28 */
    uint8_t  green_pos;    /* 0x29 */
    uint8_t  blue_size;    /* 0x2a */
    uint8_t  blue_pos;     /* 0x2b */
    uint8_t  rsvd_size;    /* 0x2c */
    uint8_t  rsvd_pos;     /* 0x2d */
    uint16_t vesapm_seg;   /* 0x2e */
    uint16_t vesapm_off;   /* 0x30 */
    uint16_t pages;        /* 0x32 */
    uint16_t vesa_attributes;  /* 0x34 */
    uint32_t capabilities;     /* 0x36 */
    uint8_t  _reserved[6]; /* 0x3a */
} __attribute__((packed));

struct apm_bios_info {
    uint16_t   version;
    uint16_t   cseg;
    uint32_t   offset;
    uint16_t   cseg_16;
    uint16_t   dseg;
    uint16_t   flags;
    uint16_t   cseg_len;
    uint16_t   cseg_16_len;
    uint16_t   dseg_len;
};

struct ist_info {
    uint32_t signature;
    uint32_t command;
    uint32_t event;
    uint32_t perf_level;
};

struct sys_desc_table {
    uint16_t length;
    uint8_t  table[14];
};

struct efi_info {
    uint32_t efi_loader_signature;
    uint32_t efi_systab;
    uint32_t efi_memdesc_size;
    uint32_t efi_memdesc_version;
    uint32_t efi_memmap;
    uint32_t efi_memmap_size;
    uint32_t efi_systab_hi;
    uint32_t efi_memmap_hi;
};

struct edid_info {
    unsigned char dummy[128];
};

struct setup_header {
    uint8_t    setup_sects;
    uint16_t   root_flags;
    uint32_t   syssize;
    uint16_t   ram_size;
#define RAMDISK_IMAGE_START_MASK    0x07FF
#define RAMDISK_PROMPT_FLAG     0x8000
#define RAMDISK_LOAD_FLAG       0x4000
    uint16_t   vid_mode;
    uint16_t   root_dev;
    uint16_t   boot_flag;
    uint16_t   jump;
    uint32_t   header;
    uint16_t   version;
    uint32_t   realmode_swtch;
    uint16_t   start_sys;
    uint16_t   kernel_version;
    uint8_t    type_of_loader;
    uint8_t    loadflags;
#define LOADED_HIGH (1<<0)
#define QUIET_FLAG  (1<<5)
#define KEEP_SEGMENTS   (1<<6)
#define CAN_USE_HEAP    (1<<7)
    uint16_t   setup_move_size;
    uint32_t   code32_start;
    uint32_t   ramdisk_image;
    uint32_t   ramdisk_size;
    uint32_t   bootsect_kludge;
    uint16_t   heap_end_ptr;
    uint8_t    ext_loader_ver;
    uint8_t    ext_loader_type;
    uint32_t   cmd_line_ptr;
    uint32_t   initrd_addr_max;
    uint32_t   kernel_alignment;
    uint8_t    relocatable_kernel;
    uint8_t    _pad2[3];
    uint32_t   cmdline_size;
    uint32_t   hardware_subarch;
    uint64_t   hardware_subarch_data;
    uint32_t   payload_offset;
    uint32_t   payload_length;
    uint64_t   setup_data;
} __attribute__((packed));

struct e820entry {
    uint64_t addr; /* Start of memory segment */
    uint64_t size; /* Size of memory segment */
    uint32_t type; /* Type of memory segment */
} __attribute__((packed));

struct edd_device_params {
    uint16_t length;
    uint16_t info_flags;
    uint32_t num_default_cylinders;
    uint32_t num_default_heads;
    uint32_t sectors_per_track;
    uint64_t number_of_sectors;
    uint16_t bytes_per_sector;
    uint32_t dpte_ptr;     /* 0xFFFFFFFF for our purposes */
    uint16_t key;      /* = 0xBEDD */
    uint8_t device_path_info_length;   /* = 44 */
    uint8_t reserved2;
    uint16_t reserved3;
    uint8_t host_bus_type[4];
    uint8_t interface_type[8];
    union {
        struct {
            uint16_t base_address;
            uint16_t reserved1;
            uint32_t reserved2;
        } __attribute__ ((packed)) isa;
        struct {
            uint8_t bus;
            uint8_t slot;
            uint8_t function;
            uint8_t channel;
            uint32_t reserved;
        } __attribute__ ((packed)) pci;
        /* pcix is same as pci */
        struct {
            uint64_t reserved;
        } __attribute__ ((packed)) ibnd;
        struct {
            uint64_t reserved;
        } __attribute__ ((packed)) xprs;
        struct {
            uint64_t reserved;
        } __attribute__ ((packed)) htpt;
        struct {
            uint64_t reserved;
        } __attribute__ ((packed)) unknown;
    } interface_path;
    union {
        struct {
            uint8_t device;
            uint8_t reserved1;
            uint16_t reserved2;
            uint32_t reserved3;
            uint64_t reserved4;
        } __attribute__ ((packed)) ata;
        struct {
            uint8_t device;
            uint8_t lun;
            uint8_t reserved1;
            uint8_t reserved2;
            uint32_t reserved3;
            uint64_t reserved4;
        } __attribute__ ((packed)) atapi;
        struct {
            uint16_t id;
            uint64_t lun;
            uint16_t reserved1;
            uint32_t reserved2;
        } __attribute__ ((packed)) scsi;
        struct {
            uint64_t serial_number;
            uint64_t reserved;
        } __attribute__ ((packed)) usb;
        struct {
            uint64_t eui;
            uint64_t reserved;
        } __attribute__ ((packed)) i1394;
        struct {
            uint64_t wwid;
            uint64_t lun;
        } __attribute__ ((packed)) fibre;
        struct {
            uint64_t identity_tag;
            uint64_t reserved;
        } __attribute__ ((packed)) i2o;
        struct {
            uint32_t array_number;
            uint32_t reserved1;
            uint64_t reserved2;
        } __attribute__ ((packed)) raid;
        struct {
            uint8_t device;
            uint8_t reserved1;
            uint16_t reserved2;
            uint32_t reserved3;
            uint64_t reserved4;
        } __attribute__ ((packed)) sata;
        struct {
            uint64_t reserved1;
            uint64_t reserved2;
        } __attribute__ ((packed)) unknown;
    } device_path;
    uint8_t reserved4;
    uint8_t checksum;
} __attribute__ ((packed));

struct edd_info {
    uint8_t device;
    uint8_t version;
    uint16_t interface_support;
    uint16_t legacy_max_cylinder;
    uint8_t legacy_max_head;
    uint8_t legacy_sectors_per_track;
    struct edd_device_params params;
} __attribute__ ((packed));

struct boot_params {
    struct screen_info screen_info;         /* 0x000 */
    struct apm_bios_info apm_bios_info;     /* 0x040 */
    uint8_t  _pad2[4];                 /* 0x054 */
    uint64_t  tboot_addr;              /* 0x058 */
    struct ist_info ist_info;           /* 0x060 */
    uint8_t  _pad3[16];                /* 0x070 */
    uint8_t  hd0_info[16]; /* obsolete! */     /* 0x080 */
    uint8_t  hd1_info[16]; /* obsolete! */     /* 0x090 */
    struct sys_desc_table sys_desc_table;       /* 0x0a0 */
    uint8_t  _pad4[144];               /* 0x0b0 */
    struct edid_info edid_info;         /* 0x140 */
    struct efi_info efi_info;           /* 0x1c0 */
    uint32_t alt_mem_k;                /* 0x1e0 */
    uint32_t scratch;      /* Scratch field! */    /* 0x1e4 */
    uint8_t  e820_entries;             /* 0x1e8 */
    uint8_t  eddbuf_entries;               /* 0x1e9 */
   uint8_t  edd_mbr_sig_buf_entries;          /* 0x1ea */
    uint8_t  _pad6[6];                 /* 0x1eb */
    struct setup_header hdr;    /* setup header */  /* 0x1f1 */
    uint8_t  _pad7[0x290-0x1f1-sizeof(struct setup_header)];
    uint32_t edd_mbr_sig_buffer[EDD_MBR_SIG_MAX];  /* 0x290 */
    struct e820entry e820_map[E820MAX];     /* 0x2d0 */
    uint8_t  _pad8[48];                /* 0xcd0 */
    struct edd_info eddbuf[EDDMAXNR];       /* 0xd00 */
    uint8_t  _pad9[276];               /* 0xeec */
} __attribute__((packed));


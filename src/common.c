/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
/*
 * This file provides routines that can be called by other libraries to access
 * platform-specific devices such as the serial port. Perhaps some day this may
 * be refactored into a more structured userspace driver model, but for now we
 * just provide the bare minimum for userspace to access basic devices such as
 * the serial port on any platform.
 */

#include <autoconf.h>
#include <assert.h>
#include <sel4/sel4.h>
#include <sel4/bootinfo.h>
#include <sel4/invocation.h>
#include <sel4platsupport/device.h>
#include <sel4platsupport/platsupport.h>
#include <sel4utils/mapping.h>
#include <simple/simple_helpers.h>
#include <vspace/vspace.h>
#include "plat_internal.h"
#include <stdio.h>
#include <vka/capops.h>
#include <string.h>
#include <sel4platsupport/arch/io.h>

#ifdef CONFIG_KERNEL_STABLE
#include <simple-stable/simple-stable.h>
#else
#include <simple-default/simple-default.h>
#endif

enum serial_setup_status {
    NOT_INITIALIZED = 0,
    START_REGULAR_SETUP,
    START_FAILSAFE_SETUP,
    SETUP_COMPLETE
};
static enum serial_setup_status setup_status = NOT_INITIALIZED;

/* Some globals for tracking initialisation variables. This is currently just to avoid
 * passing parameters down to the platform code for backwards compatibility reasons. This
 * is strictly to avoid refactoring all existing platform code */
static vspace_t *vspace = NULL;
static simple_t *simple = NULL;
static vka_t *vka = NULL;

/* To keep failsafe setup we need actual memory for a simple and a vka */
static simple_t _simple_mem;
static vka_t _vka_mem;

/* Hacky constants / data structures for a failsafe mapping */
#define DITE_HEADER_START ((seL4_Word)__executable_start - 0x1000)
static seL4_CPtr device_cap = 0;
extern char __executable_start[];


#if !(defined(CONFIG_LIB_SEL4_PLAT_SUPPORT_USE_SEL4_DEBUG_PUTCHAR) && defined(SEL4_DEBUG_KERNEL))
static void* __map_device_page(void* cookie, uintptr_t paddr, size_t size,
                               int cached, ps_mem_flags_t flags);

static ps_io_ops_t io_ops = {
    .io_mapper = {
        .io_map_fn = &__map_device_page,
        .io_unmap_fn = NULL,
    },
};

#endif


/* completely hacky way of getting a virtual address. This is used a last ditch attempt to
 * get serial device going so we can print out an error */
static seL4_Word
platsupport_alloc_device_vaddr(seL4_Word bits)
{
    seL4_Word va;

    va = DITE_HEADER_START - (1 << bits);

    /* Ensure we are aligned to bits. If not, round down. */
    va = va & ~((1 << bits) - 1);

    return va;
}

static seL4_CPtr
_platsupport_find_device_cap(seL4_Word paddr, seL4_Word page_bits, simple_t *simple, vka_t *vka)
{
    int UNUSED error;
    cspacepath_t path;
    error = vka_cspace_alloc_path(vka, &path);
    assert(!error);
    error = simple_get_frame_cap(simple, (void *) paddr, page_bits, &path);
    assert(error == seL4_NoError);
    return path.capPtr;
}

static void*
__map_device_page_failsafe(void* cookie UNUSED, uintptr_t paddr, size_t size,
                           int cached UNUSED, ps_mem_flags_t flags UNUSED)
{
    int bits = CTZ(size);
    int error;
    seL4_Word vaddr;

    if (device_cap != 0) {
        /* we only support a single page for the serial */
        for (;;);
    }
    device_cap = _platsupport_find_device_cap(paddr, bits, simple, vka);
    assert(device_cap);

    vaddr = platsupport_alloc_device_vaddr(bits);
    error =
        seL4_ARCH_Page_Map(
            device_cap,
            seL4_CapInitThreadPD,
            vaddr,
            seL4_AllRights,
            0
        );
    if (error)
        for (;;);

    assert(!error);

    return (void*)vaddr;
}

static void*
__map_device_page_regular(void* cookie UNUSED, uintptr_t paddr, size_t size,
                          int cached UNUSED, ps_mem_flags_t flags UNUSED)
{
    int bits = CTZ(size);
    void *vaddr;
    seL4_CPtr cap;

    cap = _platsupport_find_device_cap(paddr, bits, simple, vka);
    assert(cap);

    vaddr = vspace_map_pages(vspace, &cap, NULL, seL4_AllRights, 1, bits, 0);
    if (!vaddr) {
        printf("Failed to map serial device :(\n");
        for (;;);
    }
    device_cap = cap;

    return (void*)vaddr;
}

void*
__map_device_page(void* cookie, uintptr_t paddr, size_t size,
                  int cached, ps_mem_flags_t flags)
{
    if (setup_status == START_REGULAR_SETUP && vspace) {
        return __map_device_page_regular(cookie, paddr, size, cached, flags);
    } else if (setup_status == START_FAILSAFE_SETUP || !vspace) {
        return __map_device_page_failsafe(cookie, paddr, size, cached, flags);
    }
    printf("Unknown setup status!\n");
    for (;;);
}

void
platsupport_zero_globals(void)
{
    setup_status = NOT_INITIALIZED;
    device_cap = 0;
    vspace = NULL;
    vka = NULL;
    vspace = NULL;
}

/*
 * This function is designed to be called when creating a new cspace/vspace,
 * and the serial port needs to be hooked in there too.
 */
void
platsupport_undo_serial_setup(void)
{
    /* Re-initialise some structures. */
    setup_status = NOT_INITIALIZED;
    if (device_cap) {
        cspacepath_t path;
        seL4_ARCH_Page_Unmap(device_cap);
        vka_cspace_make_path(vka, device_cap, &path);
        vka_cnode_delete(&path);
        vka_cspace_free(vka, device_cap);
        device_cap = 0;
        vka = NULL;
    }
}

/* Initialise serial input interrupt. */
void
platsupport_serial_input_init_IRQ(void)
{
    __plat_serial_input_init_IRQ();
}

int
platsupport_serial_setup_io_ops(ps_io_ops_t* io_ops)
{
    int err = 0;
    if (setup_status == SETUP_COMPLETE) {
        return 0;
    }
    err = __plat_serial_init(io_ops);
    if (!err) {
        setup_status = SETUP_COMPLETE;
    }
    return err;
}

int
platsupport_serial_setup_bootinfo_failsafe(void)
{
    int err = 0;
    if (setup_status == SETUP_COMPLETE) {
        return 0;
    }
    memset(&_simple_mem, 0, sizeof(simple_t));
    memset(&_vka_mem, 0, sizeof(vka_t));
#if defined(CONFIG_LIB_SEL4_PLAT_SUPPORT_USE_SEL4_DEBUG_PUTCHAR) && defined(SEL4_DEBUG_KERNEL)
    /* only support putchar on a debug kernel */
    setup_status = SETUP_COMPLETE;
#else
    setup_status = START_FAILSAFE_SETUP;
#ifdef CONFIG_KERNEL_STABLE
    simple_stable_init_bootinfo(&_simple_mem, seL4_GetBootInfo());
#else
    simple_default_init_bootinfo(&_simple_mem, seL4_GetBootInfo());
#endif
    simple = &_simple_mem;
    vka = &_vka_mem;
    simple_make_vka(simple, vka);
#ifndef ARCH_ARM
    sel4platsupport_get_io_port_ops(&io_ops.io_port_ops, simple);
#endif
    err = platsupport_serial_setup_io_ops(&io_ops);
#endif
    return err;
}

int
platsupport_serial_setup_simple(
    vspace_t *_vspace __attribute__((unused)),
    simple_t *_simple __attribute__((unused)),
    vka_t *_vka __attribute__((unused)))
{
    int err = 0;
    if (setup_status == SETUP_COMPLETE) {
        return 0;
    }
    if (setup_status != NOT_INITIALIZED) {
        printf("Trying to initialise a partially initialised serial. Current setup status is %d\n", setup_status);
        assert(!"You cannot recover");
        return -1;
    }
#if defined(CONFIG_LIB_SEL4_PLAT_SUPPORT_USE_SEL4_DEBUG_PUTCHAR) && defined(SEL4_DEBUG_KERNEL)
    /* only support putchar on a debug kernel */
    setup_status = SETUP_COMPLETE;
#else
    /* start setup */
    setup_status = START_REGULAR_SETUP;
    vspace = _vspace;
    simple = _simple;
    vka = _vka;
#ifndef ARCH_ARM
    sel4platsupport_get_io_port_ops(&io_ops.io_port_ops, simple);
#endif
    err = platsupport_serial_setup_io_ops(&io_ops);
    /* done */
    vspace = NULL;
    simple = NULL;
    /* Don't reset vka here */
#endif
    return err;
}

static void __serial_setup()
{
    int started_regular __attribute__((unused)) = 0;
    /* this function is called if we attempt to do serial and it isn't setup.
     * we now need to handle this somehow */
    switch (setup_status) {
    case START_FAILSAFE_SETUP:
        /* we're stuck. nothing to do but make some noise and hope someone hears us */
        while (1) {
            *(char*)0 = 0;
        }
        break;
    case START_REGULAR_SETUP:
        started_regular = 1;
    case NOT_INITIALIZED:
#ifdef CONFIG_LIB_SEL4_PLAT_SUPPORT_USE_SEL4_DEBUG_PUTCHAR
        setup_status = SETUP_COMPLETE;
        printf("\nWarning: using printf before serial is set up. This only works as your\n");
        printf("printf is backed by seL4_Debug_PutChar()\n");
        started_regular = 1;
#else
        /* attempt failsafe initialization and print something out */
        platsupport_serial_setup_bootinfo_failsafe();
        if (started_regular) {
            printf("Regular serial setup failed.\n"
                   "This message coming to you courtesy of failsafe serial\n"
                   "Your vspace has been clobbered but we will keep running to get any more error output\n");
        } else {
            printf("You attempted to print before initialising the libsel4platsupport serial device!\n");
            while (1);
        }
#endif /* CONFIG_LIB_SEL4_PLAT_SUPPORT_USE_SEL4_DEBUG_PUTCHAR */
        break;
    case SETUP_COMPLETE:
        break;
    }
}

/* Squash compiler warnings (these functions are prototyped in
* libsel4rootserver).
*/
void __arch_putchar(int c) __attribute__((weak, noinline));
int __arch_getchar(void);

void
__arch_putchar(int c)
{
    if (setup_status != SETUP_COMPLETE) {
        __serial_setup();
    }
    __plat_putchar(c);
}

int
__arch_getchar(void)
{
    if (setup_status != SETUP_COMPLETE) {
        __serial_setup();
    }
    return __plat_getchar();
}

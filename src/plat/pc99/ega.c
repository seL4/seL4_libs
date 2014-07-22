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
 * Implementation of an 80x25 EGA text mode. All the functions are named with 'serial' since
 * that is what platsupport expects. This was a quick hack for a project and includes lots of
 * logic that is rather inspecific to the underlying frame buffer. If someone adds another text
 * mode frambuffer (or a linear frame buffer with a font renderer) then this should be abstracted.
 * ideally some kind of virtual terminal code could be ported over to properly deal with escape characters
 * and command codes for moving cursors etc around. But this is a basic hack for 'The machine I
 * want to test on has a screen and no serial port'
 */

#include <autoconf.h>
#include <assert.h>
#include <sel4/sel4.h>
#include "../../plat_internal.h"
#include <string.h>


#ifdef CONFIG_LIB_SEL4_PLAT_SUPPORT_X86_TEXT_EGA_PUTCHAR

/* Assumptions on the graphics mode and frame buffer location */
#define EGA_TEXT_FB_BASE 0xB8000
#define MODE_WIDTH 80
#define MODE_HEIGHT 25

/* How many lines to scroll by */
#define SCROLL_LINES 1

/* Hacky global state */
static volatile short *base_ptr = NULL;
static int cursor_x = 0;
static int cursor_y = 0;

void
__plat_serial_input_init_IRQ(void)
{
}

void
__plat_serial_init(void)
{
    /* handle an insane case where the serial might get repeatedly initialized. and
     * avoid clearing the entire screen if this is the case. This comes about due
     * to implementing device 'sharing' by just giving every process the device */
    int clear = !base_ptr;
    base_ptr = __map_device_page(EGA_TEXT_FB_BASE, 12);
    assert(base_ptr);
    /* clear the screen */
    if (clear) {
        memset(base_ptr, 0, MODE_WIDTH * MODE_HEIGHT * sizeof(*base_ptr));
        cursor_x = 0;
        cursor_y = 0;
    }
}

static void scroll(void) {
    /* number of chars we are dropping when we do the scroll */
    int clear_chars = SCROLL_LINES * MODE_WIDTH;
    /* number of chars we need to move to perform the scroll. This all the lines
     * minus however many we drop */
    int scroll_chars = MODE_WIDTH * MODE_HEIGHT - clear_chars;
    /* copy the lines up. we skip the same number of characters that we will clear, and move the
     * rest to the top. cannot use memcpy as the regions almost certainly overlap */
    memmove(base_ptr, &base_ptr[clear_chars], scroll_chars * sizeof(*base_ptr));
    /* now zero out the bottom lines that we got rid of */
    memset(&base_ptr[scroll_chars], 0, clear_chars * sizeof(*base_ptr));
    /* move the virtual cursor up */
    cursor_y -= SCROLL_LINES;
}

void
__plat_putchar(int c)
{
    /* emulate various control characters */
    if (c == '\t') {
        __plat_putchar(' ');
        while(cursor_x % 4 != 0) {
            __plat_putchar(' ');
        }
    } else if (c == '\n') {
        cursor_y ++;
        /* assume a \r with a \n */
        cursor_x = 0;
    } else if (c == '\r') {
        cursor_x = 0;
    } else {
        /* 7<<8 constructs a nice neutral grey color. */
        base_ptr[cursor_y * MODE_WIDTH + cursor_x] = ((char)c) | (7 << 8);
        cursor_x++;
    }
    if (cursor_x >= MODE_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    while (cursor_y >= MODE_HEIGHT) {
        scroll();
    }
}

int
__plat_getchar(void)
{
    assert(!"EGA framebuffer does not implement getchar");
    return 0;
}

#endif

/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <autoconf.h>
#include <sel4debug/gen_config.h>

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>
#include <utils/util.h>

static void print_slot_reg_info(char const *descr, seL4_SlotRegion *reg)
{
    if (reg->end == reg->start) {
        printf("%snone\n", descr);
    } else {
        printf("%s[%"SEL4_PRIu_word" --> %"SEL4_PRIu_word")\n",
               descr, reg->start, reg->end);
    }
}

void debug_print_bootinfo(seL4_BootInfo *info)
{
    printf("Node:                      %"SEL4_PRIu_word" of %"SEL4_PRIu_word"\n",
           info->nodeID, info->numNodes);
    printf("Domain:                    %"SEL4_PRIu_word"\n",
           info->initThreadDomain);
    printf("IPC buffer:                %p\n", info->ipcBuffer);
    printf("IO-MMU page table levels:  %"SEL4_PRIu_word"\n",
           info->numIOPTLevels);
    printf("Root cnode size:           2^%"SEL4_PRIu_word" (%lu)\n",
           info->initThreadCNodeSizeBits,
           LIBSEL4_BIT(info->initThreadCNodeSizeBits));
    print_slot_reg_info("Shared pages:              ", &info->sharedFrames);
    print_slot_reg_info("User image MMU tables:     ", &info->userImagePaging);
    print_slot_reg_info("Extra boot info pages:     ", &info->extraBIPages);
    print_slot_reg_info("User image pages:          ", &info->userImageFrames);
    print_slot_reg_info("Untyped memory:            ", &info->untyped);
    print_slot_reg_info("Empty slots:               ", &info->empty);

    printf("Extra boot info blobs:\n");
    seL4_Word offs = 0;
    while (offs < info->extraLen) {
        seL4_BootInfoHeader *h = (seL4_BootInfoHeader *)((seL4_Word)info + seL4_BootInfoFrameSize + offs);
        printf("    type: %"SEL4_PRIu_word", offset: %"SEL4_PRIu_word", len: %"SEL4_PRIu_word"\n",
               h->id, offs, h->len);
        offs += h->len;
    }

    seL4_Word numUntypeds = info->untyped.end - info->untyped.start;
    printf("Used bootinfo untyped descriptors: %"SEL4_PRIu_word" of %d\n",
           numUntypeds, CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS);
    /* Sanity check that the boot info is consistent. */
    assert(info->empty.end == LIBSEL4_BIT(info->initThreadCNodeSizeBits));
    assert(numUntypeds < CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS);
    assert(info->extraLen <= (LIBSEL4_BIT(seL4_PageBits) * (info->extraBIPages.end - info->extraBIPages.start)));

    printf("Physical memory map with available untypeds:\n");
    printf("---------------------+------+----------+-------\n");
    printf(" Phys Addr           | Size | Type     | Slot\n");
    printf("---------------------+------+----------+-------\n");

    for (seL4_Word pos = 0; pos < (seL4_Word)(-1); pos++) {
        /* Find the next descriptor according to our current position. */
        seL4_UntypedDesc *d = NULL;
        int idx = -1;
        for (int i = 0; i < numUntypeds; i++) {
            seL4_UntypedDesc *d_tmp = &info->untypedList[i];
            if (d_tmp->paddr < pos) {
                continue;
            }
            if (d && (d_tmp->paddr >= d->paddr)) {
                /* Two  descriptors for the same region are not allowed. */
                assert(d_tmp->paddr != d->paddr);
                continue;
            }
            d = d_tmp; /* Found a better match. */
            idx = i;
        }
        /* No adjacent descriptor found means there is reserved space. */
        if ((!d) || (pos < d->paddr)) {
            printf("  %#*"SEL4_PRIx_word" | -    | reserved | -\n",
                   2 + (CONFIG_WORD_SIZE / 4), pos);
            if (!d) {
                break; /* No descriptors found at all means we are done. */
            }
        }
        printf("  %#*"SEL4_PRIx_word" | 2^%-2u | %s | %"SEL4_PRIu_word"\n",
               2 + (CONFIG_WORD_SIZE / 4),
               d->paddr,
               d->sizeBits,
               d->isDevice ? "device  " : "free    ",
               info->untyped.start + idx);
        seL4_Word pos_new = d->paddr + LIBSEL4_BIT(d->sizeBits) - 1;
        assert(pos_new >= pos); /* Regions must not overflow. */
        pos = pos_new;
    }
}

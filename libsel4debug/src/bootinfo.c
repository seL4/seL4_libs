/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <autoconf.h>
#include <sel4debug/gen_config.h>

#include <stdio.h>

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
    printf("Node %"SEL4_PRIu_word" of %"SEL4_PRIu_word"\n", info->nodeID, info->numNodes);
    printf("IOPT levels:     %"SEL4_PRIu_word"\n", info->numIOPTLevels);
    printf("IPC buffer:      %p\n", info->ipcBuffer);
    print_slot_reg_info("Empty slots:     ", &info->empty);
    print_slot_reg_info("sharedFrames:    ", &info->sharedFrames);
    print_slot_reg_info("userImageFrames: ", &info->userImageFrames);
    print_slot_reg_info("userImagePaging: ", &info->userImagePaging);
    print_slot_reg_info("untypeds:        ", &info->untyped);
    printf("Initial thread domain: %"SEL4_PRIu_word"\n",
           info->initThreadDomain);
    printf("Initial thread cnode size: %"SEL4_PRIu_word"\n",
           info->initThreadCNodeSizeBits);
    printf("List of untypeds\n");
    printf("------------------\n");
    printf("Paddr    | Size   | Device\n");

    int sizes[CONFIG_WORD_SIZE] = {0};
    for (int i = 0; i < CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS && i < (info->untyped.end - info->untyped.start); i++) {
        int index = info->untypedList[i].sizeBits;
        assert(index < ARRAY_SIZE(sizes));
        sizes[index]++;
        printf("0x%"SEL4_PRIx_word" | %zu | %d\n",
               info->untypedList[i].paddr,
               (size_t)info->untypedList[i].sizeBits,
               (int)info->untypedList[i].isDevice);
    }

    printf("Untyped summary\n");
    for (int i = 0; i < ARRAY_SIZE(sizes); i++) {
        if (sizes[i] != 0) {
            printf("%d untypeds of size %d\n", sizes[i], i);
        }
    }
}

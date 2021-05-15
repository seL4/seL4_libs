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

void debug_print_bootinfo(seL4_BootInfo *info)
{

    printf("Node %lu of %lu\n", (long)info->nodeID, (long)info->numNodes);
    printf("IOPT levels:     %u\n", (int)info->numIOPTLevels);
    printf("IPC buffer:      %p\n", info->ipcBuffer);
    printf("Empty slots:     [%lu --> %lu)\n", (long)info->empty.start, (long)info->empty.end);
    printf("sharedFrames:    [%lu --> %lu)\n", (long)info->sharedFrames.start, (long)info->sharedFrames.end);
    printf("userImageFrames: [%lu --> %lu)\n", (long)info->userImageFrames.start, (long)info->userImageFrames.end);
    printf("userImagePaging: [%lu --> %lu)\n", (long)info->userImagePaging.start, (long)info->userImagePaging.end);
    printf("untypeds:        [%lu --> %lu)\n", (long)info->untyped.start, (long)info->untyped.end);
    printf("Initial thread domain: %u\n", (int)info->initThreadDomain);
    printf("Initial thread cnode size: %u\n", (int)info->initThreadCNodeSizeBits);
    printf("List of untypeds\n");
    printf("------------------\n");
    printf("Paddr    | Size   | Device\n");

    int sizes[CONFIG_WORD_SIZE] = {0};
    for (int i = 0; i < CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS && i < (info->untyped.end - info->untyped.start); i++) {
        int index = info->untypedList[i].sizeBits;
        assert(index < ARRAY_SIZE(sizes));
        sizes[index]++;
        printf("%p | %zu | %d\n", (void *)info->untypedList[i].paddr, (size_t)info->untypedList[i].sizeBits,
               (int)info->untypedList[i].isDevice);
    }

    printf("Untyped summary\n");
    for (int i = 0; i < ARRAY_SIZE(sizes); i++) {
        if (sizes[i] != 0) {
            printf("%d untypeds of size %d\n", sizes[i], i);
        }
    }
}


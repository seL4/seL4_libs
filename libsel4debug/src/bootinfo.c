/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <autoconf.h>

#include <stdio.h>

#include <sel4/sel4.h>
#include <sel4utils/util.h>

#ifdef CONFIG_USER_DEBUG_BUILD
void
debug_print_bootinfo(seL4_BootInfo *info)
{

    LOG_INFO("Node %u of %u", info->nodeID, info->numNodes);
    LOG_INFO("IOPT levels:     %u", info->numIOPTLevels);
    LOG_INFO("IPC buffer:      %p", info->ipcBuffer);
    LOG_INFO("Empty slots:     [%u --> %u)", info->empty.start, info->empty.end);
    LOG_INFO("sharedFrames:    [%u --> %u)", info->sharedFrames.start, info->sharedFrames.end);
    LOG_INFO("userImageFrames: [%u --> %u)", info->userImageFrames.start, info->userImageFrames.end);
    LOG_INFO("userImagePTs:    [%u --> %u)", info->userImagePTs.start, info->userImagePTs.end);
    LOG_INFO("untypeds:        [%u --> %u)", info->untyped.start, info->untyped.end);
    LOG_INFO("Initial thread cnode size: %u", info->initThreadCNodeSizeBits);
    LOG_INFO("List of untypeds");
    LOG_INFO("------------------");
    LOG_INFO("Paddr    | Size   ");

    int sizes[32] = {0};
    for (int i = 0; i < CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS && i < (info->untyped.end - info->untyped.start); i++) {
        if (info->untypedPaddrList[i] != 0) {
            sizes[info->untypedSizeBitsList[i]]++;
            LOG_INFO("0x%08x | %u", info->untypedPaddrList[i], info->untypedSizeBitsList[i]);
        }
    }

    LOG_INFO("Untyped summary\n");
    for (int i = 0; i < 32; i++) {
        if (sizes[i] != 0) {
            printf("%d untypeds of size %d\n", sizes[i], i);
        }
    }

    /* mainline and experimental treat device regions differently*/
#ifdef CONFIG_KERNEL_STABLE
    LOG_INFO("device untypeds: [%u --> %u)", info->deviceUntyped.start, info->deviceUntyped.end);
    LOG_INFO("List of device untypes");
    LOG_INFO("Paddr    | Size   ");
    int start = info->untyped.end - info->untyped.start;
    int size = info->deviceUntyped.end - info->deviceUntyped.start;
    for (int i = start; i < start + size; i++) {
        if (info->untypedPaddrList[i] != 0) {
            LOG_INFO("0x%08x | %u", info->untypedPaddrList[i], info->untypedSizeBitsList[i]);
        }
    }
#else
    LOG_INFO("DeviceRegions: %u\n", info->numDeviceRegions);
    printf("List of deviceRegions\n");
    printf("Paddr    | Size     | Slot Region\n");
    for (int i = 0; i < info->numDeviceRegions; i++) {
        printf("0x%08x | %08u | [%u <--> %u ]\n", info->deviceRegions[i].basePaddr,
               info->deviceRegions[i].frameSizeBits,
               info->deviceRegions[i].frames.start,
               info->deviceRegions[i].frames.end);
    }
#endif /* CONFIG_KERNEL_STABLE */

}

#endif  /* CONFIG_USER_DEBUG_BUILD */




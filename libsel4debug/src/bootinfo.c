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

    ZF_LOGI("Node %u of %u", info->nodeID, info->numNodes);
    ZF_LOGI("IOPT levels:     %u", info->numIOPTLevels);
    ZF_LOGI("IPC buffer:      %p", info->ipcBuffer);
    ZF_LOGI("Empty slots:     [%u --> %u)", info->empty.start, info->empty.end);
    ZF_LOGI("sharedFrames:    [%u --> %u)", info->sharedFrames.start, info->sharedFrames.end);
    ZF_LOGI("userImageFrames: [%u --> %u)", info->userImageFrames.start, info->userImageFrames.end);
    ZF_LOGI("userImagePaging: [%u --> %u)", info->userImagePaging.start, info->userImagePaging.end);
    ZF_LOGI("untypeds:        [%u --> %u)", info->untyped.start, info->untyped.end);
    ZF_LOGI("Initial thread domain: %u\n", info->initThreadDomain);
    ZF_LOGI("Initial thread cnode size: %u", info->initThreadCNodeSizeBits);
    ZF_LOGI("List of untypeds");
    ZF_LOGI("------------------");
    ZF_LOGI("Paddr    | Size   ");

    int sizes[32] = {0};
    for (int i = 0; i < CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS && i < (info->untyped.end - info->untyped.start); i++) {
        if (info->untypedPaddrList[i] != 0) {
            sizes[info->untypedSizeBitsList[i]]++;
            ZF_LOGI("0x%08x | %u", info->untypedPaddrList[i], info->untypedSizeBitsList[i]);
        }
    }

    ZF_LOGI("Untyped summary\n");
    for (int i = 0; i < 32; i++) {
        if (sizes[i] != 0) {
            printf("%d untypeds of size %d\n", sizes[i], i);
        }
    }

    /* mainline and experimental treat device regions differently*/
#ifdef CONFIG_KERNEL_STABLE
    ZF_LOGI("device untypeds: [%u --> %u)", info->deviceUntyped.start, info->deviceUntyped.end);
    ZF_LOGI("List of device untypes");
    ZF_LOGI("Paddr    | Size   ");
    int start = info->untyped.end - info->untyped.start;
    int size = info->deviceUntyped.end - info->deviceUntyped.start;
    for (int i = start; i < start + size; i++) {
        if (info->untypedPaddrList[i] != 0) {
            ZF_LOGI("0x%08x | %u", info->untypedPaddrList[i], info->untypedSizeBitsList[i]);
        }
    }
#else
    ZF_LOGI("DeviceRegions: %u\n", info->numDeviceRegions);
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




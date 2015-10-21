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

#ifndef CONFIG_KERNEL_STABLE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <sel4/sel4.h>

#include <simple-default/simple-default.h>

#include <sel4utils/mapping.h>

void *simple_default_get_frame_info(void *data, void *paddr, int size_bits, seL4_CPtr *frame_cap, seL4_Word *offset) {
    assert(data && paddr && frame_cap);

    seL4_BootInfo *bi = (seL4_BootInfo *) data;

    int i;
    seL4_DeviceRegion* region;

    *offset = 0;
    *frame_cap = seL4_CapNull;
    region = bi->deviceRegions;
    for(i = 0; i < bi->numDeviceRegions; i++, region++){
        seL4_Word region_start = region->basePaddr;
        seL4_Word n_caps = region->frames.end - region->frames.start;
        seL4_Word region_end = region_start + (n_caps << region->frameSizeBits);
        if(region_start <= (seL4_Word) paddr &&
           (seL4_Word) paddr < region_end &&
           region->frameSizeBits == size_bits) {

            *frame_cap =  region->frames.start + (((seL4_Word) paddr - region->basePaddr) >> region->frameSizeBits);
            i = bi->numDeviceRegions;
        }
    }

    return NULL;
}
seL4_Error simple_default_get_frame_cap(void *data, void *paddr, int size_bits, cspacepath_t *path) {
    assert(data && paddr);

    seL4_CPtr frame_cap;
    seL4_Word offset;
    simple_default_get_frame_info(data,paddr, size_bits, &frame_cap, &offset);
    if (frame_cap == seL4_CapNull) {
        return seL4_FailedLookup;
    }
    return seL4_CNode_Copy(path->root, path->capPtr, path->capDepth, seL4_CapInitThreadCNode, frame_cap, 32, seL4_AllRights);
}

seL4_CPtr simple_default_get_ut_cap(void *data, void *paddr, int size_bits) {
    assert(data && paddr);

    seL4_CPtr frame_cap;
    seL4_Word offset;

    simple_default_get_frame_info(data, paddr, size_bits, &frame_cap, &offset);

    return frame_cap;
}

void *simple_default_get_frame_mapping(void *data, void *paddr, int size_bits) {
    return NULL;
}

seL4_Error simple_default_get_irq(void *data, int irq, seL4_CNode root, seL4_Word index, uint8_t depth) {
    return seL4_IRQControl_Get(seL4_CapIRQControl, irq, root, index, depth);
}

seL4_Error simple_default_set_ASID(void *data, seL4_CPtr vspace) {
    return seL4_ARCH_ASIDPool_Assign(seL4_CapInitThreadASIDPool, vspace);
}

seL4_CPtr simple_default_get_IOPort_cap(void *data, uint16_t start_port, uint16_t end_port) {
    return seL4_CapIOPort;
}

#define INIT_CAP_BASE_RANGE (seL4_CapInitThreadASIDPool)
#if defined(CONFIG_ARCH_ARM)
    #if defined(CONFIG_IOMMU)
        #define INIT_CAP_TOP_RANGE (seL4_CapDomain - seL4_CapIOPort + INIT_CAP_BASE_RANGE)
    #else
        #define INIT_CAP_TOP_RANGE (seL4_CapDomain - seL4_CapIOSpace + INIT_CAP_BASE_RANGE)
    #endif
#elif defined(CONFIG_ARCH_IA32)
    #if defined(CONFIG_IOMMU)
        #define INIT_CAP_TOP_RANGE (seL4_CapDomain - seL4_CapInitThreadASIDPool + INIT_CAP_BASE_RANGE)
    #else
        #define INIT_CAP_TOP_RANGE (seL4_CapDomain - seL4_CapInitThreadASIDPool - 1 + INIT_CAP_BASE_RANGE)
    #endif
#endif
#define SHARED_FRAME_RANGE ((bi->sharedFrames.end - bi->sharedFrames.start) + INIT_CAP_TOP_RANGE)
#define USER_IMAGE_FRAMES_RANGE ((bi->userImageFrames.end - bi->userImageFrames.start) + SHARED_FRAME_RANGE)
#define USER_IMAGE_PTS_RANGE ((bi->userImagePTs.end - bi->userImagePTs.start) + USER_IMAGE_FRAMES_RANGE)
#define UNTYPED_RANGE ((bi->untyped.end - bi->untyped.start) + USER_IMAGE_PTS_RANGE)
#define DEVICE_RANGE (device_caps + UNTYPED_RANGE)

int simple_default_cap_count(void *data) {
    assert(data);

    seL4_BootInfo * bi = (seL4_BootInfo *) data;

    int device_caps = 0;
    int i;
    for(i = 0; i < bi->numDeviceRegions; i++) {
        device_caps += bi->deviceRegions[i].frames.end - bi->deviceRegions[i].frames.start;
    }

    return (device_caps)
           + (bi->sharedFrames.end - bi->sharedFrames.start)
           + (bi->userImageFrames.end - bi->userImageFrames.start)
           + (bi->userImagePTs.end - bi->userImagePTs.start)
           + (bi->untyped.end - bi->untyped.start)
           + INIT_CAP_TOP_RANGE; //Include all the init caps
}

seL4_CPtr simple_default_nth_cap(void *data, int n) {
    assert(data);

    seL4_BootInfo * bi = (seL4_BootInfo *) data;
    
    int i;
    int device_caps = 0;
    seL4_CPtr true_return = seL4_CapNull;
    
    for(i = 0; i < bi->numDeviceRegions; i++) {
        device_caps += bi->deviceRegions[i].frames.end - bi->deviceRegions[i].frames.start;
    }
    if(n < INIT_CAP_BASE_RANGE) {
        true_return = (seL4_CPtr) n+1;
    } else if(n < INIT_CAP_TOP_RANGE) {
        true_return = (seL4_CPtr) n+1;
        #if defined(CONFIG_ARCH_ARM)
            true_return++;
        #endif
        #ifndef CONFIG_IOMMU
            if(true_return >= seL4_CapIOSpace) {
                true_return++;
            }
        #endif
    } else if(n < SHARED_FRAME_RANGE) {
        return bi->sharedFrames.start + (n - INIT_CAP_TOP_RANGE);
    } else if(n < USER_IMAGE_FRAMES_RANGE) {
        return bi->userImageFrames.start + (n - SHARED_FRAME_RANGE);
    } else if(n < USER_IMAGE_PTS_RANGE) {
        return bi->userImagePTs.start + (n - USER_IMAGE_FRAMES_RANGE);
    } else if(n < UNTYPED_RANGE) {
        return bi->untyped.start + (n - USER_IMAGE_PTS_RANGE);
    } else if(n < DEVICE_RANGE) {
        i = 0;
        int current_count = 0;
        while((bi->deviceRegions[i].frames.end - bi->deviceRegions[i].frames.start) + current_count < (n - UNTYPED_RANGE)) {
            current_count += bi->deviceRegions[i].frames.end - bi->deviceRegions[i].frames.start;
            i++;
        }
        return bi->deviceRegions[i].frames.start + (n - UNTYPED_RANGE - current_count);
    }

    return true_return;
}

seL4_CPtr simple_default_init_cap(void *data, seL4_CPtr cap_pos) {
    return (seL4_CPtr) cap_pos;
}

uint8_t simple_default_cnode_size(void *data) {
    assert(data);

    return ((seL4_BootInfo *)data)->initThreadCNodeSizeBits;
}

int simple_default_untyped_count(void *data) {
    assert(data);

    return ((seL4_BootInfo *)data)->untyped.end - ((seL4_BootInfo *)data)->untyped.start;
}

seL4_CPtr simple_default_nth_untyped(void *data, int n, uint32_t *size_bits, uint32_t *paddr) {
    assert(data && size_bits && paddr);

    seL4_BootInfo *bi = (seL4_BootInfo *)data;

    if(n < (bi->untyped.end - bi->untyped.start)) {
        if(paddr != NULL) {
            *paddr = bi->untypedPaddrList[n];
        }
        if(size_bits != NULL) {
            *size_bits = bi->untypedSizeBitsList[n];
        }
        return bi->untyped.start + (n);
    }

    return seL4_CapNull;
}

int simple_default_userimage_count(void *data) {
    assert(data);

    return ((seL4_BootInfo *)data)->userImageFrames.end - ((seL4_BootInfo *)data)->userImageFrames.start;
}

seL4_CPtr simple_default_nth_userimage(void *data, int n) {
    assert(data);

    seL4_BootInfo *bi = (seL4_BootInfo *)data;

    if(n < (bi->userImageFrames.end - bi->userImageFrames.start)) {
        return bi->userImageFrames.start + (n);
    }

    return seL4_CapNull;
}

void simple_default_print(void *data) {
    assert(data);

    seL4_BootInfo *info = (seL4_BootInfo *)data;

    /* Parse boot info from kernel. */
    printf("Node ID: %d (of %d)\n",info->nodeID, info->numNodes);
    printf("initThreadCNode size: %d slots\n", (1 << info->initThreadCNodeSizeBits) );

    printf("\n--- Capability Details ---\n");
    printf("Type              Start      End\n");
    printf("Empty             0x%08x 0x%08x\n", info->empty.start, info->empty.end);
    printf("Shared frames     0x%08x 0x%08x\n", info->sharedFrames.start, info->sharedFrames.end);
    printf("User image frames 0x%08x 0x%08x\n", info->userImageFrames.start,
            info->userImageFrames.end);
    printf("User image PTs    0x%08x 0x%08x\n", info->userImagePTs.start, info->userImagePTs.end);
    printf("Untypeds          0x%08x 0x%08x\n", info->untyped.start, info->untyped.end);

    printf("\n--- Untyped Details ---\n");
    printf("Untyped Slot       Paddr      Bits\n");
    for (int i = 0; i < info->untyped.end-info->untyped.start; i++) {
        printf("%3d     0x%08x 0x%08x %d\n", i, info->untyped.start+i, info->untypedPaddrList[i],
                info->untypedSizeBitsList[i]);
    }

    printf("\n--- Device Regions: %d ---\n", info->numDeviceRegions);
    printf("Device Addr     Size Start      End\n");
    for (int i = 0; i < info->numDeviceRegions; i++) {
        printf("%2d 0x%08x %d 0x%08x 0x%08x\n", i,
                                                info->deviceRegions[i].basePaddr,
                                                info->deviceRegions[i].frameSizeBits,
                                                info->deviceRegions[i].frames.start,
                                                info->deviceRegions[i].frames.end);
    }
}

void simple_default_init_bootinfo(simple_t *simple, seL4_BootInfo *bi) {
    assert(simple);
    assert(bi);

    simple->data = bi;
    simple->frame_info = &simple_default_get_frame_info;
    simple->frame_cap = &simple_default_get_frame_cap;
    simple->frame_mapping = &simple_default_get_frame_mapping;
    simple->IOPort_cap = &simple_default_get_IOPort_cap;
    simple->irq = &simple_default_get_irq;
    simple->ASID_assign = &simple_default_set_ASID;
    simple->cap_count = &simple_default_cap_count;
    simple->nth_cap = &simple_default_nth_cap;
    simple->init_cap = &simple_default_init_cap;
    simple->cnode_size = &simple_default_cnode_size;
    simple->untyped_count = &simple_default_untyped_count;
    simple->nth_untyped = &simple_default_nth_untyped;
    simple->userimage_count = &simple_default_userimage_count;
    simple->nth_userimage = &simple_default_nth_userimage;
    simple->print = &simple_default_print;
}

#endif

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

#ifdef CONFIG_KERNEL_STABLE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <sel4/sel4.h>

#include <simple-stable/simple-stable.h>

#include <sel4utils/mapping.h>
#include <sel4utils/util.h>
#include <vka/kobject_t.h>

void *simple_stable_get_frame_info(void *data, void *paddr, int size_bits, seL4_CPtr *frame_cap, seL4_Word *ut_offset) {
    assert(data && frame_cap && ut_offset);

    seL4_BootInfo *bi = (seL4_BootInfo *) data;

    *frame_cap = 0;
    *ut_offset = 0;

    seL4_Word paddr_word = (seL4_Word) paddr;
    int i, offset;

    for(i = 0; i < bi->untyped.end - bi->untyped.start; i++){
        if(bi->untypedPaddrList[i] <= paddr_word && 
           paddr_word <= bi->untypedPaddrList[i] + BIT(bi->untypedSizeBitsList[i]) - 1 &&
           bi->untypedSizeBitsList[i] >= size_bits){
            *frame_cap = i+bi->untyped.start;
            *ut_offset = paddr_word - bi->untypedPaddrList[i];
            return NULL;
        }
    }
    offset = i;

    for(i = 0; i < bi->deviceUntyped.end - bi->deviceUntyped.start; i++){
        if(bi->untypedPaddrList[i + offset] <= paddr_word && 
           paddr_word <= bi->untypedPaddrList[i + offset] + BIT(bi->untypedSizeBitsList[i + offset]) - 1 &&
           bi->untypedSizeBitsList[i + offset] >= size_bits){
            *frame_cap = i+bi->deviceUntyped.start;
            *ut_offset = paddr_word - bi->untypedPaddrList[i+offset];
            return NULL;
        }
    }

    return NULL;
}

seL4_Error simple_stable_get_frame_cap(void *data, void *paddr, int size_bits, cspacepath_t *path) {
    assert(data);

    seL4_CPtr cap;
    seL4_Word offset;

    simple_stable_get_frame_info(data,paddr,size_bits,&cap,&offset);

    return seL4_Untyped_RetypeAtOffset(cap, kobject_get_type(KOBJECT_FRAME, size_bits), offset, size_bits,
                                       path->root, path->dest, path->destDepth, path->offset, 1);
}

seL4_CPtr simple_stable_get_ut_cap(void *data, void *paddr, int size_bits) {
    assert(data);

    seL4_CPtr frame_cap;
    seL4_Word offset;

    simple_stable_get_frame_info(data,paddr,size_bits,&frame_cap, &offset);

    return frame_cap;
}

void *simple_stable_get_frame_mapping(void *data, void *paddr, int size_bits) {
    return NULL;
}

seL4_Error simple_stable_get_irq(void *data, int irq, seL4_CNode root, seL4_Word index, uint8_t depth) {
    return seL4_IRQControl_Get(seL4_CapIRQControl, irq, root, index, depth);
}

seL4_Error simple_stable_set_ASID(void *data, seL4_CPtr vspace) {
    return seL4_NoError;
}

seL4_CPtr simple_stable_get_IOPort_cap(void *data, uint16_t start_port, uint16_t end_port) {
    return seL4_CapIOPort;
}

#define INIT_CAP_BASE_RANGE (4)
#if defined(CONFIG_ARCH_ARM)
    #if defined(CONFIG_IOMMU)
        #define INIT_CAP_TOP_RANGE (3 + INIT_CAP_BASE_RANGE)
        seL4_CPtr caps[] = {1,2,3,4,6,7,9};
    #else
        #define INIT_CAP_TOP_RANGE (2 + INIT_CAP_BASE_RANGE)
        seL4_CPtr caps[] = {1,2,3,4,7,9};
    #endif
#elif defined(CONFIG_ARCH_IA32)
    #if defined(CONFIG_IOMMU)
        #define INIT_CAP_TOP_RANGE (6 + INIT_CAP_BASE_RANGE)
        seL4_CPtr caps[] = {1,2,3,4,5,6,7,8,9,10};
    #else
        #define INIT_CAP_TOP_RANGE (5 + INIT_CAP_BASE_RANGE)
        seL4_CPtr caps[] = {1,2,3,4,5,7,8,9,10};
    #endif
#endif

#define SHARED_FRAME_RANGE ((bi->sharedFrames.end - bi->sharedFrames.start) + INIT_CAP_TOP_RANGE)
#define USER_IMAGE_FRAMES_RANGE ((bi->userImageFrames.end - bi->userImageFrames.start) + SHARED_FRAME_RANGE)
#define USER_IMAGE_PTS_RANGE ((bi->userImagePTs.end - bi->userImagePTs.start) + USER_IMAGE_FRAMES_RANGE)
#define UNTYPED_RANGE ((bi->untyped.end - bi->untyped.start) + USER_IMAGE_PTS_RANGE)
#define DEVICE_RANGE ((bi->deviceUntyped.end - bi->deviceUntyped.start) + UNTYPED_RANGE)

int simple_stable_cap_count(void *data) {
    seL4_BootInfo * bi = (seL4_BootInfo *) data;

    return INIT_CAP_TOP_RANGE //Include all the init caps
           + (bi->sharedFrames.end - bi->sharedFrames.start)
           + (bi->userImageFrames.end - bi->userImageFrames.start)
           + (bi->userImagePTs.end - bi->userImagePTs.start)
           + (bi->untyped.end - bi->untyped.start)
           + (bi->deviceUntyped.end - bi->deviceUntyped.start);
}

seL4_CPtr simple_stable_nth_cap(void *data, int n) {
    seL4_BootInfo * bi = (seL4_BootInfo *) data;
    seL4_CPtr true_return = seL4_CapNull;

    if(n < INIT_CAP_TOP_RANGE) {
        true_return = caps[n];
    } else if(n < SHARED_FRAME_RANGE) {
        true_return = bi->sharedFrames.start + (n - INIT_CAP_TOP_RANGE);
    } else if(n < USER_IMAGE_FRAMES_RANGE) {
        true_return = bi->userImageFrames.start + (n - SHARED_FRAME_RANGE);
    } else if(n < USER_IMAGE_PTS_RANGE) {
        true_return = bi->userImagePTs.start + (n - USER_IMAGE_FRAMES_RANGE);
    } else if(n < UNTYPED_RANGE) {
        true_return = bi->untyped.start + (n - USER_IMAGE_PTS_RANGE);
    } else if(n < DEVICE_RANGE) { 
        true_return = bi->deviceUntyped.start + (n - UNTYPED_RANGE);
    }

    return true_return;
}

seL4_CPtr simple_stable_init_cap(void *data, seL4_CPtr cap_pos) {
    return (seL4_CPtr) cap_pos;
}

uint8_t simple_stable_cnode_size(void *data) {
    assert(data);

    return ((seL4_BootInfo *)data)->initThreadCNodeSizeBits;
}

int simple_stable_untyped_count(void *data) {
    assert(data);

    return ((seL4_BootInfo *)data)->untyped.end - ((seL4_BootInfo *)data)->untyped.start;
}

seL4_CPtr simple_stable_nth_untyped(void *data, int n, uint32_t *size_bits, uint32_t *paddr) {
    assert(data && size_bits);

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
int simple_stable_userimage_count(void *data) {
    assert(data);

    return ((seL4_BootInfo *)data)->userImageFrames.end - ((seL4_BootInfo *)data)->userImageFrames.start;
}

seL4_CPtr simple_stable_nth_userimage(void *data, int n) {
    assert(data);

    seL4_BootInfo *bi = (seL4_BootInfo *)data;

    if(n < (bi->userImageFrames.end - bi->userImageFrames.start)) {
        return bi->userImageFrames.start + (n);
    }

    return seL4_CapNull;
}

#ifdef CONFIG_IOMMU
seL4_Error simple_stable_get_iospace(void *data, uint16_t domainID, uint16_t deviceID, cspacepath_t *path) {
    return seL4_CNode_Mint(path->root, path->capPtr, path->capDepth, seL4_CapInitThreadCNode, seL4_CapIOSpace, 32, seL4_AllRights, (seL4_CapData_t){.words = {((uint32_t)domainID << 16) | (uint32_t)deviceID}});
}
#endif

void simple_stable_print(void *data) {
    assert(data);
    seL4_BootInfo *info = (seL4_BootInfo *) data;

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

    int i,offset;
    for (i = 0; i < info->untyped.end-info->untyped.start; i++) {
        printf("%3d     0x%08x 0x%08x %d\n", i,
                                             info->untyped.start+i,
                                             info->untypedPaddrList[i],
                                             info->untypedSizeBitsList[i]);
    }

    offset = i;
    printf("\n--- Device Untyped Details ---\n");
    printf("Untyped Slot       Paddr      Bits\n");
    for (i = 0; i < info->deviceUntyped.end-info->deviceUntyped.start; i++) {
        printf("%3d     0x%08x 0x%08x %d\n", i,
                                             info->deviceUntyped.start+i,
                                             info->untypedPaddrList[i+offset],
                                             info->untypedSizeBitsList[i+offset]);
    }
}



void simple_stable_init_bootinfo(simple_t *simple, seL4_BootInfo *bi) {
    assert(simple);
    assert(bi);

    simple->data = bi;
    simple->frame_info = &simple_stable_get_frame_info;
    simple->frame_cap = &simple_stable_get_frame_cap;
    simple->frame_mapping = &simple_stable_get_frame_mapping;
    simple->irq = &simple_stable_get_irq;
    simple->ASID_assign = &simple_stable_set_ASID;
    simple->IOPort_cap = &simple_stable_get_IOPort_cap;
    simple->cap_count = &simple_stable_cap_count;
    simple->nth_cap = &simple_stable_nth_cap;
    simple->init_cap = &simple_stable_init_cap;;
    simple->cnode_size = &simple_stable_cnode_size;
    simple->untyped_count = &simple_stable_untyped_count;
    simple->nth_untyped = &simple_stable_nth_untyped;
    simple->userimage_count = &simple_stable_userimage_count;
    simple->nth_userimage = &simple_stable_nth_userimage;
#ifdef CONFIG_IOMMU
    simple->iospace = &simple_stable_get_iospace;
#endif
    simple->print = &simple_stable_print;
}

/* Backwards compatbility reasons means this function should stay here
 * One day someone may remove it, use simple_stable_init_bootinfo from now on
 */
void simple_cdt_init_bootinfo(simple_t *simple, seL4_BootInfo *bi) {
    simple_stable_init_bootinfo(simple,bi);
}

#endif

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
#include <stdlib.h>
#include <assert.h>

#include <sel4/sel4.h>
#include <sel4debug/debug.h>
#include <simple-default/simple-default.h>

#include <vspace/page.h>

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

seL4_Error simple_default_set_ASID(void *data, seL4_CPtr vspace) {
    return seL4_ARCH_ASIDPool_Assign(seL4_CapInitThreadASIDPool, vspace);
}

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
           + (bi->userImagePaging.end - bi->userImagePaging.start)
           + (bi->untyped.end - bi->untyped.start)
           + seL4_NumInitialCaps; //Include all the init caps
}

seL4_CPtr simple_default_nth_cap(void *data, int n) {
    assert(data);

    seL4_BootInfo * bi = (seL4_BootInfo *) data;
    size_t shared_frame_range = bi->sharedFrames.end - bi->sharedFrames.start + seL4_NumInitialCaps;
    size_t user_img_frame_range = bi->userImageFrames.end - bi->userImageFrames.start + shared_frame_range;
    size_t user_img_paging_range = bi->userImagePaging.end - bi->userImagePaging.start + user_img_frame_range;
    size_t untyped_range = bi->untyped.end - bi->untyped.start + user_img_paging_range;

    int i;
    int device_caps = 0;
    seL4_CPtr true_return = seL4_CapNull;

    for(i = 0; i < bi->numDeviceRegions; i++) {
        device_caps += bi->deviceRegions[i].frames.end - bi->deviceRegions[i].frames.start;
    }

    size_t device_range = device_caps + untyped_range;

    if (n < seL4_CapInitThreadASIDPool) {
        true_return = (seL4_CPtr) n+1;
    } else if (n < seL4_NumInitialCaps) {
        true_return = (seL4_CPtr) n+1;
#if defined(CONFIG_ARCH_ARM)
        true_return++;
#endif
#ifndef CONFIG_IOMMU
        if(true_return >= seL4_CapIOSpace) {
            true_return++;
        }
#endif
    } else if (n < shared_frame_range) {
        return bi->sharedFrames.start + (n - seL4_NumInitialCaps);
    } else if (n < user_img_frame_range) {
        return bi->userImageFrames.start + (n - shared_frame_range);
    } else if (n < user_img_paging_range) {
        return bi->userImagePaging.start + (n - user_img_frame_range);
    } else if (n < untyped_range) {
        return bi->untyped.start + (n - user_img_paging_range);
    } else if (n < device_range) {
        i = 0;
        int current_count = 0;
        while((bi->deviceRegions[i].frames.end - bi->deviceRegions[i].frames.start) +
               current_count < (n - untyped_range)) {
            current_count += bi->deviceRegions[i].frames.end - bi->deviceRegions[i].frames.start;
            i++;
        }
        return bi->deviceRegions[i].frames.start + (n - untyped_range - current_count);
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
    if (data == NULL) {
        ZF_LOGE("Data is null!");
    }

    seL4_BootInfo *info = (seL4_BootInfo *)data;
    debug_print_bootinfo(info);
}

void simple_default_init_bootinfo(simple_t *simple, seL4_BootInfo *bi) {
    assert(simple);
    assert(bi);

    simple->data = bi;
    simple->frame_info = &simple_default_get_frame_info;
    simple->frame_cap = &simple_default_get_frame_cap;
    simple->frame_mapping = &simple_default_get_frame_mapping;
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
    simple_default_init_arch_simple(&simple->arch_simple, NULL);
}

